#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//* Type of lock
#define READ 0
#define WRITE 1

//* Special lock case
#define NA 0
#define MOVE 1

//* Save the entries of the inode_table that have been locked
typedef struct locks_to_unlock{
	int rdArray[INODE_TABLE_SIZE];
	int rdSize;
	int wrArray[INODE_TABLE_SIZE];
	int wrSize;
} locks_to_unlock;


int check(locks_to_unlock *ltu, int inumber){
	for(int i = 0; i < ltu->rdSize; i++){
		if(inumber == ltu->rdArray[i]){
			return SUCCESS;
		}
	}
	for(int i = 0; i < ltu->wrSize; i++){
		if(inumber == ltu->wrArray[i]){
			return SUCCESS;
		}
	}
	return FAIL;	
}


/*
 * Locks the inumber and adds to the ltu
 * Input:
 *  - ltu: Struct that has the inumber that are locked
 *  - inumber
 * 	- mode: type of lock
 * 	- command: used for special cases
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
void lock_inode(locks_to_unlock *ltu, int inumber, int mode, int command){
	if(command == MOVE){
		if(check(ltu, inumber) != FAIL){
				return;
		}
	}

	if(mode == WRITE){
		wrLock(inumber);
		ltu->wrArray[(ltu->wrSize)++] = inumber;
	
	}
	else if(mode == READ){
		rdLock(inumber);
		ltu->rdArray[(ltu->rdSize)++] = inumber;
	}

}

void ltu_unlock(locks_to_unlock *ltu){
	int i;
	for(i = 0; i < ltu->rdSize; i++){
		unlock(ltu->rdArray[i]);
	}

	for(i = 0; i < ltu->wrSize; i++){
		unlock(ltu->wrArray[i]);
	}
}



/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * 	- ltu: Struct that has the inumber that are locked
 * 	- mode: type of lock
 * 	- command: used for special cases
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, locks_to_unlock *ltu, int mode, int command) {
	char *saveptr;
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";

	strcpy(full_path, name);

	//* Start at root node
	int current_inumber = FS_ROOT;

	//* Use for copy
	type nType;
	union Data data;

	char *path = strtok_r(full_path, delim, &saveptr);

	if(path == NULL && mode == WRITE){
		lock_inode(ltu, current_inumber, WRITE, command);
	}else{
		lock_inode(ltu, current_inumber, READ, command);
	}

	//* Get ROOT inode data
	inode_get(current_inumber, &nType, &data);

	//* search for all sub nodes 
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL){
		path = strtok_r(NULL, delim, &saveptr);

		//* If it's the last node of the search
		if(path == NULL && mode == WRITE){
			lock_inode(ltu, current_inumber, WRITE, command);
		}
		else{
			lock_inode(ltu, current_inumber, READ, command);
		}

		inode_get(current_inumber, &nType, &data);
	}

	return current_inumber;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * 	- ltu: Struct that has the inumber that are locked
 * Returns: SUCCESS or FAIL
 */
int create_aux(char *name, type nodeType, locks_to_unlock *ltu){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	
	//* use for copy 
	type pType;
	union Data pdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, ltu, WRITE, NA);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);

	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		return FAIL;
	}

	//* Lock the created node
	lock_inode(ltu, child_inumber, WRITE, NA);

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * 	- ltu: Struct that has the inumber that are locked
 * Returns: SUCCESS or FAIL
 */
int delete_aux(char *name, locks_to_unlock *ltu){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, ltu, WRITE, NA);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
				
		return FAIL;
	}
	
	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);
	
	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		return FAIL;
	}

	//* Lock the node that is going to be deleted
	lock_inode(ltu, child_inumber, WRITE, NA);
	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		return FAIL;
	}

	return SUCCESS;
}


/*
 * Moves a node given a path.
 * Input:
 *  - origin: path of node
 * 	- destiny: path of node
 * 	- ltu: Struct that has the inumber that are locked
 * Returns: SUCCESS or FAIL
 */
int move_aux(char *origin, char *destiny, locks_to_unlock *ltu){
	//* Use for copy
	char origin_copy[MAX_FILE_NAME];
	char destiny_copy[MAX_FILE_NAME];
	union Data pdata;
	type pType;
	//* Origin variables
	int originParent_inumber, originChild_inumber;
	char *originParent_name, *originChild_name;
	//* Destiny variables
	int destinyParent_inumber; 
	char *destinyParent_name, *destinyChild_name;


	//* Get the names
	strcpy(origin_copy, origin);
	split_parent_child_from_path(origin_copy, &originParent_name, &originChild_name);

	strcpy(destiny_copy, destiny);
	split_parent_child_from_path(destiny_copy, &destinyParent_name, &destinyChild_name);

	//* Defines an order for the locks
	if(strcmp(originParent_name, destinyParent_name) < 0){
		originParent_inumber = lookup(originParent_name, ltu, WRITE, MOVE);
		destinyParent_inumber = lookup(destinyParent_name, ltu, WRITE, MOVE);
	}
	else{
		destinyParent_inumber = lookup(destinyParent_name, ltu, WRITE, MOVE);
		originParent_inumber = lookup(originParent_name, ltu, WRITE, MOVE);
	}

	if (originParent_inumber == FAIL) {
		printf("failed to move %s, invalid parent dir %s\n",
		        originChild_name, originParent_name);
		return FAIL;
	}

	if (destinyParent_inumber == FAIL) {
		printf("failed to move %s, target dir doesn't exist %s\n",
		        destinyChild_name, destinyParent_name);
		return FAIL;
	}

	
	inode_get(originParent_inumber, &pType, &pdata);
	if(pType != T_DIRECTORY) {
		printf("failed to move %s, parent %s is not a dir\n",
		        originChild_name, originParent_name);
		return FAIL;
	}

	originChild_inumber = lookup_sub_node(originChild_name, pdata.dirEntries);	
	if (originChild_inumber == FAIL) {
		printf("could not move %s, does not exist in dir %s\n",
		       originChild_name, originParent_name);
		return FAIL;
	}

	if(check(ltu, originChild_inumber) != FAIL){
		printf("failed to move %s, cannot move to inside of itslef\n",
		    originChild_name);
		return FAIL;
	}
	

	//* Lock the node that is going to be deleted and moved
	lock_inode(ltu, originChild_inumber, WRITE, MOVE);

	inode_get(destinyParent_inumber, &pType, &pdata);
	if(pType != T_DIRECTORY) {
		printf("failed to move %s, %s is not a dir\n",
		        originChild_name, destinyParent_name);
		return FAIL;
	}

	if (lookup_sub_node(destinyChild_name, pdata.dirEntries) != FAIL) {
		printf("failed to move %s, already exists in dir %s\n",
		       destinyChild_name, destinyParent_name);
		return FAIL;
	}


	//* Removes from from thr original dir
	if (dir_reset_entry(originParent_inumber, originChild_inumber) == FAIL) {
		printf("failed to remove %s from dir %s\n",
		       originChild_name, originParent_name);
		return FAIL;
	}

	//* Adds to the destiny dir
	if (dir_add_entry(destinyParent_inumber, originChild_inumber, destinyChild_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       destinyChild_name, destinyParent_name);
		return FAIL;
	}

	return SUCCESS;
}


/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}


int lookfor(char *name){
	int exit_state;
	locks_to_unlock ltu;
	ltu.rdSize = 0;
	ltu.wrSize = 0;

	exit_state = lookup(name, &ltu, READ, NA);
	ltu_unlock(&ltu);
	return exit_state;
}
	

int create(char *name, type nodeType){
	int exit_state;
	locks_to_unlock ltu;
	ltu.rdSize = 0;
	ltu.wrSize = 0;

	exit_state = create_aux(name, nodeType, &ltu);
	ltu_unlock(&ltu);
	return exit_state;
}


int delete(char *name){
	int exit_state;
	locks_to_unlock ltu;
	ltu.rdSize = 0;
	ltu.wrSize = 0;

	exit_state = delete_aux(name, &ltu);
	ltu_unlock(&ltu);
	return exit_state;
}


int move(char *origin, char *destiny){
	int exit_state = 0;
	locks_to_unlock ltu;
	ltu.rdSize = 0;
	ltu.wrSize = 0;

	exit_state = move_aux(origin, destiny, &ltu);
	ltu_unlock(&ltu);
	return exit_state;
}
