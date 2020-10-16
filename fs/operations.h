#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs(char *strategy);
void destroy_fs(char *strategy);
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, char *strategy);
int delete(char *name, char *strategy);
int lookup(char *name, char *strategy);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
