#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){
	int parent_inumber, child_inumber, node_array[100], indice=0;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	parent_inumber = lookup_aux(parent_name, (int *) &node_array, &indice);
	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		while (indice > -1){
		    unlock(node_array[indice]);
		    indice--;
		}
		return FAIL;
	}
	//write_lock(parent_inumber);
	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
		//unlock(parent_inumber);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
        //unlock(parent_inumber);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
        unlock(child_inumber);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
        unlock(child_inumber);
		return FAIL;
	}
    while (indice > -1){
        unlock(node_array[indice]);
        indice--;
    }
    unlock(child_inumber);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

	int parent_inumber, child_inumber, node_array[100], indice=0;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup_aux(parent_name, (int *) &node_array, &indice);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);
	write_lock(child_inumber);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
        unlock(child_inumber);
		return FAIL;
	}

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
        unlock(child_inumber);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
        unlock(child_inumber);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
        while (indice > -1){
            unlock(node_array[indice]);
            indice--;
        }
        unlock(child_inumber);
		return FAIL;
	}
    while (indice > -1){
        unlock(node_array[indice]);
        indice--;
    }
    unlock(child_inumber);
	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name){
    char full_path[MAX_FILE_NAME];
    char delim[] = "/";

    strcpy(full_path, name);

    /* start at root node */
    int current_inumber = FS_ROOT;

    /* use for copy */
    type nType;
    union Data data;
    /* get root inode data */
    inode_get(current_inumber, &nType, &data);

    char *saveptr;
    char *path = strtok_r(full_path, delim, &saveptr);

    /* search for all sub nodes */
    while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
        read_lock(current_inumber);
        inode_get(current_inumber, &nType, &data);
        unlock(current_inumber);
        path = strtok_r(NULL, delim, &saveptr);
    }

    return current_inumber;
}


int lookup_aux(char *name, int *nodeArray, const int *indice){//, int *nodeArray, int indice) { passar a ser auxiliar para poder registar que nos foram locked, usar array de inteiros (criado no create)
    // com o inumber de cada no, e *variavel de indice que incrementa
    // e criar um lookup de topo com parametros adequados
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;
	/* get root inode data */
	int indice_aux=0;
	nodeArray[0]=FS_ROOT;
	read_lock(FS_ROOT);
	inode_get(current_inumber, &nType, &data);

    char *saveptr, *saveptr1;
    char *path = strtok_r(full_path, delim, &saveptr);
    char *path_aux = strtok_r(full_path, delim, &saveptr1);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {//verificar se é o ultimo path (sera NULL) e se for faer um write lock caso contrario fazer so read lock
	    indice_aux++;
        path_aux = strtok_r(NULL, delim, &saveptr1);
	    if (path_aux == NULL){
	        write_lock(current_inumber);
	        nodeArray[indice_aux]=current_inumber;
	    } else{
	        read_lock(current_inumber);
            nodeArray[indice_aux]=current_inumber;
	    }
		inode_get(current_inumber, &nType, &data);
		path = strtok_r(NULL, delim, &saveptr);
	}
	indice = &indice_aux;

    return current_inumber;
}

int move(char *name_origin, char *name_destiny){
    int originNumber = lookup(name_origin);
    if(originNumber < 0)
        return 1;
    int destinyNumber = lookup(name_destiny);
    if(destinyNumber >= 0)
        return 2;
    inode_move(originNumber, name_origin, name_destiny);
    return 0;
}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}
