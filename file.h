/*
** Author:    David Ekstrom
** Professor: Prof. Carithers
** Class:     Systems Programming
** Date:      4/25/2022
*/

#ifndef _FILE_H
#define _FILE_H


typedef struct f_node *file_t;

typedef struct f_space *fdata_t;

char *fileBuffer;

file_t rootDir;

/**
** Name:        create_fnode
**
** Desc:        populate file node with data
**
** @param name  name of file or directory
** @param wData flag for linking data to the file node
**
** @return      pointer to file node
*/
file_t create_fnode(char name[16], int wData);

/**
** Name:        add_file
**
** Desc:        adds file node to end of the directory linked list
**
** @param f     fnode at the start of a directories linked list
** @param name  name of file or directory
** @param wData flag for linking data to the file node
**
** @return      pointer to the newly added fnode
*/
file_t add_file(file_t f, char name[16], int wData);

/**
** Name:        add_to_dir
**
** Desc:        adds file node as a subfile/child to a directory
**
** @param f     parent directory file node
** @param name  name of file or directory
** @param wData flag for linking data to the file node
**
** @return      pointer to the newly added fnode
*/
file_t add_to_dir(file_t f, char name[16], int wData);

/**
** Name:        file_delete
**
** Desc:        deletes a file and makes sure the file structure is continous
**
** @param f     file to be deleted
**
*/
void file_delete(file_t f);


/**
** Name:        search_invasive
**
** Desc:        searches all possible files and directory from starting directory
**              commonly used with rootdir to search all
**
** @param f     starting directory
** @param name  name of file or dir being searched for
**
** @return      pointer to found file node or NULL
*/
file_t search_invasive(file_t f, char name[16]);


/**
** Name:        search_subfiles
**
** Desc:        wrapper for search_dir
**              ensures searching of a directory's subfiles
**
**
** @param f     directory
** @param name  name of file or dir being searched for
**
** @return      pointer to found file node or NULL
*/
file_t search_subfiles(file_t f, char name[16]);


/**
** Name:        print_name
**
** Desc:        actually gets the name
**
** @param f     file or directory
**
** @return      name of file or directory
*/
char *print_name(file_t f);

/**
** Name:        get_file_contents
**
** Desc:        gets the parent of a file or directory
**
** @param f     file or directory
**
** @return      parent file or directory
*/
void print_file_contents(file_t f);

/**
** Name:        get_parent
**
** Desc:        gets the parent of a file or directory
**
** @param f     file or directory
**
** @return      parent file or directory
*/
file_t get_parent(file_t f);

/**
** Name:        get_prev
**
** Desc:        gets the prev of a file or directory
**
** @param f     file or directory
**
** @return      prev file or directory in linked list
*/
file_t get_prev(file_t f);


/**
** Name:        preorder_traversal
**
** Desc:        preforms a preorder traversal
**              used for tree command in file system
**
** @param f     directory to begin preorder traversal
*/
void preorder_traversal(file_t f);


/**
** Name:        _file_init
**
** Desc:        initializes file system for use
**
**
** @param run   flag for starting the file system
*/
void _file_init( int run );

#endif

