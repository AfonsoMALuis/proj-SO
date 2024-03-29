#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
int lookup_aux(char *name, int *nodeArray, const int *indice);
int move(char *name_origin, char *name_destiny);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
