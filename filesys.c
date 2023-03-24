/*
** Author:    David Ekstrom
** Professor: Prof. Carithers
** Class:     Systems Programming
** Date:      4/28/2022
*/

#include "file.h"
#include "cio.h"
#include "common.h"
#include "kmem.h"
#include "kdefs.h"



//active directory of the file system. ie directory user is in
static file_t activeDir;


/**
** Name:        search
**
** Desc:        search for a file in current and all following directories
**
** @param f     starting directory
** @param name  name of file/dir being searched for
*/
void search(file_t f, char name[16]){

	__cio_printf("\nsearching for %s in %s\n", name, print_name(f)); 

	file_t found = search_invasive(f, name);

	
	if (found != NULL) {
		__cio_printf("found file:\t %s\n", print_name(found));
	}
	else {
		__cio_printf("unable to find %s\n", name);
	}
	
}

/**
** Name:        search_d
**
** Desc:        search for a file in current directory
**
** @param f     current directory
** @param name  name of file/dir being searched for
*/
void search_d(file_t f, char name[16]){

	
        __cio_printf("\nsearching for %s in %s\n", name, print_name(f));

        file_t found = search_subfiles(f, name);

        if (found != NULL) {
                __cio_printf("found file:\t %s\n", print_name(found));
        }
        else {
                __cio_printf("unable to find %s\n", name);
        }
	
}


/**
** Name:        make_file
**
** Desc:        creates file in file structure
**
** @param f     directory file should be added too
** @param name  name of file being created
*/
void make_file( file_t f, char name[16] ){
	if (strcmp(name, "\n") == 0){
		__cio_puts("name cannot be empty");
		return;
	}

	add_to_dir(f, name, 1);
}

/**
** Name:        make_dir
**
** Desc:        creates directory in file structure
**
** @param f     directory new directory should be added too
** @param name  name of directory being created
*/

void make_dir( file_t f, char name[16] ){
        if (strcmp(name, "\n") == 0){
                __cio_puts("name cannot be empty");
                return;
        }
 
        add_to_dir(f, name, 0);
}

/**
** Name:        change_dir
**
** Desc:        changes the active directory
**
** @param currentDir	current directory
** @param name  	name of directory being changed to
*/
void change_dir(file_t currentDir, char name[16]){
	file_t temp = search_subfiles(currentDir, name);
	if (temp != NULL){
		activeDir = temp;
	}
	else{
		__cio_printf("unable to find %s\n", name);
	}
}


/**
** Name:        go_up
**
** Desc:        moves the active directory to the current directory's parent
**
** @param currentDir    current directory
*/
void go_up(file_t currentDir){
	file_t temp;
	if (get_prev(currentDir)){
		temp = get_prev(currentDir);
		while(get_prev(temp) != NULL){
			temp = get_prev(temp);
		}
		temp = get_parent(temp);
	}
	else{
		temp = get_parent(currentDir);
	}
	
	if (temp != NULL){
                activeDir = temp;
        }
	else{
                __cio_printf("unable to find parent of %s\n", print_name(currentDir));
        }
}

/**
** Name:        delete_file
**
** Desc:        deletes a file or directoy from the file structure
**
** @param currentDir    current directory
** @param name          name of file/dir being deleted
*/
void delete_file(file_t currDir, char name[16]){
	file_t found = search_subfiles(currDir, name);
	file_delete(found);
}

/**
** Name:        print_contents
**
** Desc:        prints the contents of a file
**
** @param currentDir    current directory
** @param name          name of file being printed
*/
void print_contents(file_t currDir, char name[16]){
	file_t found = search_subfiles(currDir, name);
	print_file_contents(found);
}


/**
** Name:        print_help
**
** Desc:        prints the help message
*/
void print_help( void ){
	__cio_puts("help     prints this message\n");
	__cio_puts("tree -t  prints the file tree\n");
	__cio_puts("gu       goes up one directory\n");
	__cio_puts("lf       lists files in directory\n");
	__cio_puts("mf <filename> |make file\n");
	__cio_puts("md <dirname>  |make dir\n");
	__cio_puts("cd <dirname>  |change dir\n");
	__cio_puts("del <name>    |delete file\n");
}


/**
** Name:        get_command
**
** Desc:        gets command from user for use then calls relevant function
**
** @param dir	active directory
**
** @return 	1 if there is another command; 0 if user wants to exit
*/
int get_command( file_t dir ){
	__cio_printf("%s > ", print_name(dir));
	int size = __cio_gets(fileBuffer, 1024);

	if (size > 1024 ) size = 1024;
	char command[size];

	strcpy(command, fileBuffer);
	__memclr(fileBuffer, SZ_SLICE);
	

	if (strcmp(command, "exit\n") == 0){
                return 0;
        }

	char tok[3][32];
	int offset = 0;
	for (int i = 0; i < 3; ++i){
		offset += __strtok(tok[i], command+offset, ' ');
		++offset;
		//TODO make sure starting ' ' char is gone
		//__cio_printf("Token %d: %s | ", i, tok[i]);
	}
	
	//__cio_puts("\n");

	//replaces newline or space with EOL char
        size = strlen(tok[1]);
	if (tok[1][size-1] ==  '\n' || tok[1][size-1] == ' ' ){ 
		tok[1][size-1] = '\0';
	}
	
	
	if (strcmp(tok[0], "mf") == 0){       
		make_file(dir, tok[1]);
	}
	else if (strcmp(tok[0], "md") == 0){
                make_dir(dir, tok[1]);
        }
	else if (strcmp(tok[0], "cd") == 0){
                change_dir(dir, tok[1]);
        }
	else if (strcmp(tok[0], "tree\n") == 0){
                preorder_traversal(rootDir);
        }
	else if(strcmp(tok[0], "help\n") == 0){
		print_help();
	}
	else if(strcmp(tok[0], "gu\n") == 0){
               	go_up(dir);
        }
	else if(strcmp(tok[0], "del") == 0){
                delete_file(dir, tok[1]);
        }
	else if(strcmp(tok[0], "cat") == 0){
        	print_contents(dir, tok[1]);
	}
	else if(strcmp(tok[0], "lf\n") == 0){
                preorder_traversal(dir);
        }
	else if(strcmp(tok[0], "\n") == 0){
                //pass through
        }
	else{
		__cio_puts("Unrecognized command\n");
	}

	return 1;
}



/**
** Name:        file_system
**
** Desc:        runs the file system for the user to interact with
*/
void file_system( void ){

	// active directory should always start at root
	activeDir = rootDir;

	__cio_puts("\n");

	// run the command loop
	while (get_command(activeDir) != 0);

}	
