#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookfor(char *name);
int move(char *origin, char *dest);
int print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
