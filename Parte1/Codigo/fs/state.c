#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "state.h"
#include "../tecnicofs-api-constants.h"

inode_t inode_table[INODE_TABLE_SIZE];

/*sync variables*/
int syncStrat;
void *state_lock;

/*Initiates the sync method inserted in the input*/
void Strat_Init(char *strat){
    if(strcmp(strat,"mutex") == 0){
        syncStrat = MSYNC;
    }
    else if(strcmp(strat,"rwlock") == 0){
        syncStrat = RWSYNC;
    }
    else if(strcmp(strat,"nosync") == 0){
        syncStrat = NOSYNC;
    }
    else{
        fprintf(stderr, "Error: %s is not an available sync strategy\n",strat);
        exit(EXIT_FAILURE);
    }
}

void *Lock_Init(){
    if(syncStrat == MSYNC){
        pthread_mutex_t* mlock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(mlock, NULL);
        return (void*) mlock;
    }
    else if(syncStrat == RWSYNC){
        pthread_rwlock_t* rwlock = (pthread_rwlock_t*) malloc(sizeof(pthread_rwlock_t));
        pthread_rwlock_init(rwlock, NULL);
        return (void*) rwlock;
    }
    return NULL;
}

void Destroy_Lock(void* lock){
    free(lock);
}

/*Lock code using the defined sync strategy*/
void Lock(void* lock ,int rw){
    if(syncStrat == MSYNC){
        pthread_mutex_lock((pthread_mutex_t*) lock);
    }
    else if(syncStrat == RWSYNC){
        if(rw == READ){
            pthread_rwlock_rdlock((pthread_rwlock_t*) lock);
        }
        else if(rw == WRITE){
            pthread_rwlock_wrlock((pthread_rwlock_t*) lock);
        }
    }
}



/*Unlock code using the defined sync strategy*/
void Unlock(void* lock){
    if(syncStrat == MSYNC){
        pthread_mutex_unlock((pthread_mutex_t*) lock);
    }
    else if(syncStrat == RWSYNC){
        pthread_rwlock_unlock((pthread_rwlock_t*) lock);
    }
}



/*
 * Sleeps for synchronization testing.
 */
void insert_delay(int cycles) {
    for (int i = 0; i < cycles; i++) {}
}


/*
 * Initializes the i-nodes table.
 */
void inode_table_init(char *syncStrat) {
    Strat_Init(syncStrat);
    state_lock = Lock_Init();

    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        inode_table[i].nodeType = T_NONE;
        inode_table[i].data.dirEntries = NULL;
        inode_table[i].data.fileContents = NULL;
    }
}

/*
 * Releases the allocated memory for the i-nodes tables.
 */

void inode_table_destroy() {
    Destroy_Lock(state_lock);
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        if (inode_table[i].nodeType != T_NONE) {
            /* as data is an union, the same pointer is used for both dirEntries and fileContents */
            /* just release one of them */
	  if (inode_table[i].data.dirEntries)
            free(inode_table[i].data.dirEntries);
        }
    }
}

/*
 * Creates a new i-node in the table with the given information.
 * Input:
 *  - nType: the type of the node (file or directory)
 * Returns:
 *  inumber: identifier of the new i-node, if successfully created
 *     FAIL: if an error occurs
 */
int inode_create(type nType) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    for (int inumber = 0; inumber < INODE_TABLE_SIZE; inumber++) {
        if (inode_table[inumber].nodeType == T_NONE) {
            inode_table[inumber].nodeType = nType;

            if (nType == T_DIRECTORY) {
                /* Initializes entry table */
                inode_table[inumber].data.dirEntries = malloc(sizeof(DirEntry) * MAX_DIR_ENTRIES);
                
                for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
                    inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
                }
            }
            else {
                inode_table[inumber].data.fileContents = NULL;
            }
            return inumber;
        }
    }
    return FAIL;
}

/*
 * Deletes the i-node.
 * Input:
 *  - inumber: identifier of the i-node
 * Returns: SUCCESS or FAIL
 */
int inode_delete(int inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_delete: invalid inumber\n");
        return FAIL;
    } 

    inode_table[inumber].nodeType = T_NONE;
    /* see inode_table_destroy function */
    if (inode_table[inumber].data.dirEntries)
        free(inode_table[inumber].data.dirEntries);
    return SUCCESS;
}

/*
 * Copies the contents of the i-node into the arguments.
 * Only the fields referenced by non-null arguments are copied.
 * Input:
 *  - inumber: identifier of the i-node
 *  - nType: pointer to type
 *  - data: pointer to data
 * Returns: SUCCESS or FAIL
 */
int inode_get(int inumber, type *nType, union Data *data) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    //!Critical
    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_get: invalid inumber %d\n", inumber);
        return FAIL;
    }

    if (nType)
        *nType = inode_table[inumber].nodeType;

    if (data)
        *data = inode_table[inumber].data;

    return SUCCESS;
}


/*
 * Resets an entry for a directory.
 * Input:
 *  - inumber: identifier of the i-node
 *  - sub_inumber: identifier of the sub i-node entry
 * Returns: SUCCESS or FAIL
 */
int dir_reset_entry(int inumber, int sub_inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid inumber\n");
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_reset_entry: can only reset entry to directories\n");
        return FAIL;
    }

    if ((sub_inumber < FREE_INODE) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid entry inumber\n");
        return FAIL;
    }

    
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == sub_inumber) {
            inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
            inode_table[inumber].data.dirEntries[i].name[0] = '\0';
            return SUCCESS;
        }
    }
    return FAIL;
}


/*
 * Adds an entry to the i-node directory data.
 * Input:
 *  - inumber: identifier of the i-node
 *  - sub_inumber: identifier of the sub i-node entry
 *  - sub_name: name of the sub i-node entry 
 * Returns: SUCCESS or FAIL
 */
int dir_add_entry(int inumber, int sub_inumber, char *sub_name) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid inumber\n");
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_add_entry: can only add entry to directories\n");
        return FAIL;
    }

    if ((sub_inumber < 0) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid entry inumber\n");
        return FAIL;
    }

    if (strlen(sub_name) == 0 ) {
        printf("inode_add_entry: \
               entry name must be non-empty\n");
        return FAIL;
    }
    
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == FREE_INODE) {
            inode_table[inumber].data.dirEntries[i].inumber = sub_inumber;
            strcpy(inode_table[inumber].data.dirEntries[i].name, sub_name);
            return SUCCESS;
        }
    }
    return FAIL;
}


/*
 * Prints the i-nodes table.
 * Input:
 *  - inumber: identifier of the i-node
 *  - name: pointer to the name of current file/dir
 */
void inode_print_tree(FILE *fp, int inumber, char *name) {
    if (inode_table[inumber].nodeType == T_FILE) {
        fprintf(fp, "%s\n", name);
        return;
    }

    if (inode_table[inumber].nodeType == T_DIRECTORY) {
        fprintf(fp, "%s\n", name);
        for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
            if (inode_table[inumber].data.dirEntries[i].inumber != FREE_INODE) {
                char path[MAX_FILE_NAME];
                if (snprintf(path, sizeof(path), "%s/%s", name, inode_table[inumber].data.dirEntries[i].name) > sizeof(path)) {
                    fprintf(stderr, "truncation when building full path\n");
                }
                inode_print_tree(fp, inode_table[inumber].data.dirEntries[i].inumber, path);
            }
        }
    }
}
