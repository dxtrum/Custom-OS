/*
** Author:    David Ekstrom
** Professor: Prof. Carithers
** Class:     Systems Programming
** Date:      4/25/2022
*/

#include "file.h"
#include "common.h"
#include "kmem.h"
#include "cio.h"
#include "kdefs.h"
#include "filesys.h"

#define MEMSPACE 2	// pages to allocate for file data
#define MEMMAX 4	// maximum pages allowed
#define BLOCKSPACE 248	// space reserved for file data inside of a datablock;

/*
** File node struct
** holds data needed for file structure
** uses doubley linked lists
**	one to keep track of dependancies (subfiles) for each directory
**	one to keep track of files in a directory
*/
typedef struct f_node{
	char name[16];	//16 bytes
	
	fdata_t startAddy; //24
	int dataLength;	//28
	
	int depth;	//32
	
	// doubley linked list for children files
	struct f_node *subFiles; //40
	struct f_node *parent; //48
	
	// doubley linked list of other files in the directory
	struct f_node *next; //56
	struct f_node *prev; //64
}f_info;

//list of pointers to free file nodes 
static file_t fNodeList;

//declared here for use
static void free_fnode(file_t fn);


/*
** File block struct
** 256 bytes
** data - 248 bytes of data for a file
** next - pointer to next block of data if file isn't done
**	  otherwise NULL
*/
typedef struct f_space{
	char data[BLOCKSPACE];
	struct f_space *next;
}datablock;


// list of free data blocks
static fdata_t dataBlocks;


// buffer for reading in data for files
char *fileBuffer;


/**
** Name: 	alloc_file_buffer
**
** Desc:	Allocate space for the file buffer
**	
** @return 0 on failure, 1 on success
*/
static int alloc_file_buffer( void ){

	//allocate a slice for the buffer
	fileBuffer = (char*) _km_slice_alloc();
        
	// if buffer cannot be allocated exit
	if (fileBuffer == NULL) {
                return 0;
        }

        //clear allocated space
        __memclr (fileBuffer, SZ_SLICE);
 
 	return 1;
}


/**
** Name:      	free_dblock
**
** Desc:        free a datablock
**
** @param t 	data block to be freed
*/
static void free_dblock(fdata_t t){
	assert(t != NULL);

	//add new datablock to top of stack
        t->next = dataBlocks;
        dataBlocks = t;
}


/**
** Name:        alloc_mem_space
**
** Desc:        Allocates pages of memory based on MEMSPACE or MEMMAX 
**
** @return      number of data blocks allocated
*/
static int alloc_mem_space( void ){ 

	// number of blocks cannot exceed MEMMAX
	int blocks = (MEMSPACE < MEMMAX) ? MEMSPACE : MEMMAX;
	fdata_t data = (fdata_t) _km_page_alloc( blocks );
	if (data == NULL){
		return 0;
	}

	// clear out memory for page
	__memclr (data, SZ_PAGE);

	// create data blocks from page
	for (int i = 0; i < (SZ_SLICE / sizeof(datablock)); ++i){
                //free blocks one at a time
		free_dblock(&data[i]);
        }
        return (SZ_PAGE / sizeof(datablock));

}

/**
** Name:        create_file_block
**
** Desc:        creates a (or many) file block(s) from a free data block(s)
**		depending on size
**
** @param contents	text data of file
** @param size		size of data
**
** @return      pointer to starting block
*/
fdata_t create_file_block( char *contents, int size ){
	fdata_t new, tmp, first;

        //is memory allocated
        if (dataBlocks == NULL) {

                //allocate if memory can be allocated
                if (!alloc_mem_space()){
                        //memory cannot be allocated return null
                        return(NULL);
                }
        }
	

	int blockCount = (size / BLOCKSPACE);

	for (int i = 0; i <= blockCount; ++i){
		
		//get allocated fileBlock from top of stack
		new = dataBlocks;

		//set stack to current top
	        dataBlocks = new->next;

		//remove pointer to stack
        	new->next = NULL;

		//only want to access next 248 bytes
		char* temp = &contents[i*BLOCKSPACE];

		strcpy(new->data, temp);


		// if not first iteration set new block as previous's next block
		if (i > 0) {
			tmp->next = new;
		}
		// otherwise set first pointer
		else {
			first = new;
		}

		//update tmp to new to use as previous
		tmp = new;
	}

        return first;
}


/**
** Name:        link_file
**
** Desc:        links the data of a file to a fnode 
**
** @param f	fnode to link to file
**
** @return      pointer to first linked file block 
*/
fdata_t link_file( file_t f ){
	//gets file data from user
	__cio_puts("\nEnter file contents: ");
	int size = __cio_gets(fileBuffer, 1024);
	__cio_puts("\n");

	// fnode gets necesary data and fileblock(s) are created
	f->dataLength = size;
	f->startAddy = create_file_block(fileBuffer, size);
	__memclr (fileBuffer, SZ_SLICE);
	return f->startAddy;
}


/**
** Name:        alloc_new_fnode
**
** Desc:        allocates space for file nodes from a slice 
**
** @param f	fnode to link to file
**
** @return      pointer to first linked file block 
*/
static int alloc_new_fnode( void ){
	file_t new = (file_t) _km_slice_alloc();
	if (new == NULL) {
		return 0;
	}

	//clear allocated space
	__memclr (new, SZ_SLICE);

	for (int i = 0; i < (SZ_SLICE / sizeof(f_info)); ++i){
		free_fnode(&new[i]);
	}
	return (SZ_SLICE / sizeof(f_info));
}


/**
** Name:        create_fnode
**
** Desc:        populate file node with data 
**
** @param name	name of file or directory
** @param wData	flag for linking data to the file node
**
** @return      pointer to file node
*/
file_t create_fnode(char name[16], int wData){
	file_t new;

	//is memory allocated
	if (fNodeList == NULL) {
		
		//allocate if memory can be allocated
		if (!alloc_new_fnode()){
			//memory cannot be allocated return null
			return(NULL);
		}
	}

	//remove fnode from stack
	new = fNodeList;
	fNodeList = new->next;

	//populate fnode with data
	new->next = new->subFiles = new->prev = new->parent = NULL;
	new->depth = 0;
	__strcpy(new->name, name);

	// if flag is set link file
	if (wData == 0){
		new->dataLength = NULL;
		new->startAddy = NULL;
	} else {
		link_file(new);
	}

	return new;
}




/**
** Name:        free_fnode
**
** Desc:        frees a file mode and adds it to the stack
**
** @param f	fnode to free
*/
static void free_fnode(file_t f){
	assert(f != NULL);

	f->next = fNodeList;
	fNodeList = f;
}


/**
** Name:        add_file
**
** Desc:        adds file node to end of the directory linked list
**
** @param f	fnode at the start of a directories linked list
** @param name	name of file or directory
** @param wData	flag for linking data to the file node
**
** @return      pointer to the newly added fnode
*/
file_t add_file(file_t f, char name[16], int wData){
        if (f == NULL){
                return NULL;
        }

	// continue until at end of linked list
        while (f->next){
                f = f->next;
        }

	// create file node
        f->next = create_fnode(name, wData);
	
	//set prev value to current f node
	f->next->prev = f;

	//set proper depth
	f->next->depth = f->depth; 
        return f->next;
}

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
file_t add_to_dir(file_t f, char name[16], int wData){
        if (f == NULL){
                return NULL;
        }

	// if a subfile already exists add it to the end of the linked list
        if (f->subFiles){
                return add_file(f->subFiles, name, wData);
        }
        else{
                
		f->subFiles = create_fnode(name, wData);
		
		f->subFiles->parent = f; 
		
		int depth = f->depth;
		f->subFiles->depth = ++depth;
        }
	
        return f->subFiles;
}



/**
** Name:        delete_contents
**
** Desc:        deletes a file block and frees its memory
**
** @param f     fblock to be deleted
**
*/

void delete_contents(fdata_t f){
	__memset(f->data, BLOCKSPACE, '\0');

	free_dblock(f);
}

/**
** Name:        file_delete
**
** Desc:        deletes a file and makes sure the file structure is continous
**
** @param f     file to be deleted
**
*/
void file_delete(file_t f){
	
	//if f has subfiles print error and exit quietly
	if (f->subFiles){
		__cio_printf("cannot delete %s, has subfiles\n");
		return;
	}

	if (f->startAddy != NULL){
		delete_contents(f->startAddy);
	}
	
	//if f has a prev set set the prev's next to f's next
	if (f->prev){
		f->prev->next = f->next;
		f->next->prev = f;
	}
	else if (f->parent){
		f->parent->subFiles = f->next;
		f->next->parent =f;
	}

	free_fnode(f);
}


/**
** Name:        search_invasive
**
** Desc:        searches all possible files and directory from starting directory
**		commonly used with rootdir to search all
**
** @param f     starting directory
** @param name  name of file or dir being searched for
**
** @return      pointer to found file node or NULL
*/
file_t search_invasive(file_t f, char name[16]){
	if (f != NULL){
		
		file_t tmp;
		//printf("search \t%s\n", print_name(f));

		char currName[16];
		strcpy(currName, f->name);

		// if names match return the file node				
		if (strcmp(currName, name) == 0){
			return f;
		}
		
		//search files in directories
		if (f->subFiles){
                        tmp = search_invasive(f->subFiles, name);
			if (tmp != NULL){
				strcpy(currName, tmp->name);
				
				//if names match return the file node
				if (strcmp(currName, name) == 0){
                        		return tmp;
				}
			}
                }
		
		//search next file in directory
		if (f->next){
			tmp = search_invasive(f->next, name);
			if (tmp != NULL){
				strcpy(currName, tmp->name);

				//if names match return the file node
        	                if (strcmp(currName, name) == 0){
                        		return tmp;
                		}
			}
		}
	}
	return NULL;
}

/**
** Name:        search_dir
**
** Desc:        searches all files in given directory
**
** @param f     directory
** @param name  name of file or dir being searched for
**
** @return      pointer to found file node or NULL
*/
file_t search_dir(file_t f, char name[16]){
	if (f != NULL){

                file_t tmp;
                //printf("search \t%s\n", print_name(f));

                char currName[16];
                strcpy(currName, f->name);

		// if names are the same return the fnode
                if (strcmp(currName, name) == 0){
                        return f;
                }


		//search the next file in the directory
                if (f->next){
                        tmp = search_dir(f->next, name);
                        if (tmp != NULL){
                                strcpy(currName, tmp->name);

				// if names are the same return the fnode
                                if (strcmp(currName, name) == 0){
                                return tmp;
                                }
                        }
                }
        }
        return NULL;
}


/**
** Name:        search_subfiles
**
** Desc:        wrapper for search_dir 
**		ensures searching of a directory's subfiles
**	
**
** @param f     directory
** @param name  name of file or dir being searched for
**
** @return      pointer to found file node or NULL
*/
file_t search_subfiles(file_t f, char name[16]){
	return search_dir(f->subFiles, name);
}


/**
** Name:        print_name
**
** Desc:        actually gets the name
**
** @param f    	file or directory
**
** @return      name of file or directory
*/
char *print_name(file_t f){
	return f->name;
}


/**
** Name:        get_parent
**
** Desc:        gets the parent of a file or directory
**
** @param f     file or directory
**
** @return      parent file or directory
*/
file_t get_parent(file_t f){
	return f->parent;
}

/**
** Name:        get_file_contents
**
** Desc:        gets the parent of a file or directory
**
** @param f     file or directory
**
** @return      parent file or directory
*/
void print_file_contents(file_t f){
        fdata_t temp = f->startAddy;
	// if fnode has linked file print it
        if (temp){
                __cio_printf(" %s", temp->data);
	}
	else{
                __cio_printf("file has no contents\n");
        }

}


/**
** Name:        get_prev
**
** Desc:        gets the prev of a file or directory
**
** @param f     file or directory
**
** @return      prev file or directory in linked list
*/
file_t get_prev(file_t f){
        return f->prev;
}



/**
** Name:        preorder_traversal
**
** Desc:        preforms a preorder traversal
**		used for tree command in file system
**
** @param f     directory to begin preorder traversal
*/
void preorder_traversal(file_t f){
	
	//  print spaces based on depth 
	for (int i = 0; i < f->depth; i++){
		__cio_printf("  ");	    
	}
	
	//  print name of current node
     	__cio_printf("%s", f->name);


	// if fnode has linked file print it
	if (f->startAddy){
		__cio_printf(" %s", f->startAddy->data);
	}else{
		__cio_printf("\n");
	}
	
	// check directory subfiles
	if (f->subFiles != NULL) {
		preorder_traversal(f->subFiles);
	}

	// check next file in directory
	if (f->next != NULL){
		preorder_traversal(f->next);
        }
}


/**
** Name:       	_file_init
**
** Desc:        initializes file system for use
**              
**
** @param run 	flag for starting the file system
*/
void _file_init(int run) {

	__cio_puts("\nDavX File System:");

	// allocate space for file sys
	fNodeList = NULL;
	if (alloc_new_fnode() < 1){
		__cio_puts("FILE INIT FAIL");
	}

	// allocate memory blocks
	dataBlocks = NULL;
	if (alloc_mem_space() < 1){
		__cio_puts("FILE INIT FAIL");
	}

	// allocated slice for file buffer
	fileBuffer = NULL;
	if (alloc_file_buffer() != 1){
		__cio_puts("NO SPACE FOR FILE BUFFER");
	}
	
	//create rootDir for sustained use
	rootDir = create_fnode("rootdir", 0);

	// if flag is set start the file system
	if (run > 0) {
		file_system();
	}

	__cio_puts("done");
}

