#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "state.h"
#include "../../tecnicofs-api-constants.h"

inode_t inode_table[INODE_TABLE_SIZE];


/*
 * Locks mutexes/wrlocks
 */
void write_lock (int inumber) {
    int ver = pthread_rwlock_trywrlock(&inode_table[inumber].rwlock);
    if ( ver == EBUSY) {
    } else if (ver == EINVAL) {
        perror("Error locking inode_t wrlock!");
        exit(1);
    }
}

/*
 * Locks mutexes/rdlocks
 */
void read_lock(int inumber) {
    int ver = pthread_rwlock_tryrdlock(&inode_table[inumber].rwlock);
    if ( ver == EBUSY) {
    } else if (ver == EINVAL) {
        perror("Error locking inode_t rdlock!");
        exit(1);
    }
}

/*
 * Unlocks mutexes/rwlocks
 */
void unlock (int inumber){
    if (pthread_rwlock_unlock(&inode_table[inumber].rwlock) != 0) {
        perror("Error unlocking inode_t rwlock!");
        exit(1);
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
void inode_table_init() {
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        inode_table[i].nodeType = T_NONE;
        inode_table[i].data.dirEntries = NULL;
        inode_table[i].data.fileContents = NULL;
        if (pthread_rwlock_init(&inode_table[i].rwlock, NULL) != 0) {
            perror("Error initializing the rwlocks!\n");
            exit(1);
        }
    }
}

/*
 * Releases the allocated memory for the i-nodes tables.
 */

void inode_table_destroy() {
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        if (inode_table[i].nodeType != T_NONE) {
            /* as data is an union, the same pointer is used for both dirEntries and fileContents */
            /* just release one of them */
            if (inode_table[i].data.dirEntries)
                free(inode_table[i].data.dirEntries);
        }
        if (pthread_rwlock_destroy(&inode_table[i].rwlock) != 0) {
            perror("Error destroying the rwlocks!\n");
            exit(1);
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
int inode_create(type nType){
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    for (int inumber = 0; inumber < INODE_TABLE_SIZE; inumber++) {
        if (pthread_rwlock_trywrlock(&inode_table[inumber].rwlock) != 0) {
            continue;
        }
        if (inode_table[inumber].nodeType == T_NONE) {
            inode_table[inumber].nodeType = nType;

            if (nType == T_DIRECTORY) {
                /* Initializes entry table */
                inode_table[inumber].data.dirEntries = malloc(sizeof(DirEntry) * MAX_DIR_ENTRIES);

                for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
                    inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
                }
            } else {
                inode_table[inumber].data.fileContents = NULL;
            }
            unlock(inumber);
            return inumber;
        }
        unlock(inumber);
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
    if (pthread_rwlock_tryrdlock(&inode_table[inumber].rwlock) != 0) {
    }
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_get: invalid inumber %d\n", inumber);
        unlock(inumber);
        return FAIL;
    }

    if (nType)
        *nType = inode_table[inumber].nodeType;

    if (data)
        *data = inode_table[inumber].data;

    unlock(inumber);
    return SUCCESS;
}

int inode_move(int inumber, char *oldname, char *newName){
    for (int i = 0; i<INODE_TABLE_SIZE-1; i++) {
        if (pthread_rwlock_trywrlock(&inode_table[i].rwlock) != 0){
        }
        puts(inode_table[i].data.dirEntries->name);
        if (inode_table[i].data.dirEntries->name[0] == '\0')
            break;
        int lenName = strlen(oldname);
        if (lenName <= 0)
            break;
        for (int j = 0; j < lenName; j++) {
            if (strcmp(inode_table[i].data.dirEntries->name, &oldname[j]) == 0) {
                strcpy(inode_table[i].data.dirEntries->name, &newName[j]);
                break;
            }
        }
        unlock(inumber);
    }
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
    write_lock(inumber);
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid inumber\n");
        unlock(inumber);
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_reset_entry: can only reset entry to directories\n");
        unlock(inumber);
        return FAIL;
    }

    if ((sub_inumber < FREE_INODE) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid entry inumber\n");
        unlock(inumber);
        return FAIL;
    }


    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == sub_inumber) {
            inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
            inode_table[inumber].data.dirEntries[i].name[0] = '\0';
            unlock(inumber);
            return SUCCESS;
        }
    }
    unlock(inumber);
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
    write_lock(inumber);
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid inumber\n");
        unlock(inumber);
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_add_entry: can only add entry to directories\n");
        unlock(inumber);
        return FAIL;
    }

    if ((sub_inumber < 0) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid entry inumber\n");
        unlock(inumber);
        return FAIL;
    }

    if (strlen(sub_name) == 0 ) {
        printf("inode_add_entry: entry name must be non-empty\n");
        unlock(inumber);
        return FAIL;
    }

    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == FREE_INODE) {
            inode_table[inumber].data.dirEntries[i].inumber = sub_inumber;
            strcpy(inode_table[inumber].data.dirEntries[i].name, sub_name);
            unlock(inumber);
            return SUCCESS;
        }
    }
    unlock(inumber);
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
