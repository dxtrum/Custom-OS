/**
** @file bitmap.c
**
** @author CSCI-452 class of 20215
** @author Chris Barnes
**
** This file contains all of the code necessary to use bitmaps.
** Bit maps are the main way that the screen gets cleared and
** are also used for character input and output.
*/

#define	SP_KERNEL_SRC

#include "common.h"
#include "vga.h"
#include "font.h"
#include "bitmap.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

// 256-color driver


/**
** Name:  create_bitmap
**
** Create a bitmap that begins at (x, y) and width long and height tall.
** Set its foreground and background colors to those specified.
** On creation, fill in the entire bitmap to the color of the background.
**
** @param bmp           Pointer to where the bitmap information
**                      should be stored
** @param x             The x coordinate
** @param y             The y coordinate
** @param width         The width of the bitmap
** @param height        The height of the bitmap
** @param fore_color    The foreground color of the bitmap
** @param back_color    The background color of the bitmap
*/
void create_bitmap(bmp_t* bmp, int x, int y, int width, int height,
        unsigned fore_color, unsigned back_color) {

    bmp->width = width;
    bmp->height = height;
    bmp->fore_color = fore_color; 
    bmp->back_color = back_color;

    int size = width * height;
    int offset = y * 320 + x;
    unsigned char* raster = (unsigned char*) (16uL * get_fb_segment() + offset);
    
    unsigned char* tmp = raster;
    for(int row = 0; row < height; row++) {
        __memset(tmp, width, back_color);
        tmp += DEFAULT_WIDTH;
    }

    bmp->raster = raster;
}

/**
** Name:  create_bitmap_safe
**
** Create a bitmap that begins at (x, y) and width long and height tall.
** Set its foreground and background colors to those specified.
** do not draw over any of the pixels currently there
**
** @param bmp           Pointer to where the bitmap information
**                      should be stored
** @param x             The x coordinate
** @param y             The y coordinate
** @param width         The width of the bitmap
** @param height        The height of the bitmap
** @param fore_color    The foreground color of the bitmap
** @param back_color    The background color of the bitmap
*/
void create_bitmap_safe(bmp_t* bmp, int x, int y, int width, int height,
        unsigned fore_color, unsigned back_color) {

    bmp->width = width;
    bmp->height = height;
    bmp->fore_color = fore_color;
    bmp->back_color = back_color;

    int size = width * height;
    int offset = y * 320 + x;
    unsigned char* raster = (unsigned char*) (16uL * get_fb_segment() + offset);

    bmp->raster = raster;
}


/**
** Name:  draw_bitmap
**
** Draw a given bitmap staring at the x and y coordinate
**
** @param bmp   the bitmap to draw
** @param x     the x coordinate
** @param y     the y coordinate
**
*/
void draw_bitmap(bmp_t* bmp, int x, int y) {
    int row, col;
    int width = bmp->width;
    int height = bmp->height;

    for(row = y; row < y + height; row++) {
        for(col = x; col < x + width; col++) {
            write_pixel8(col, row, bmp->raster[col + row * width]);
        }
    }
}


/**
** Name:  draw_character()
**
** This function draws a character at the x an y coordinate as specified
**
** @param x     The x coordinate
** @param y     The y coordinate
** @param c     The color of the character
*/
void draw_character(int x, int y, char c) {
    bmp_t bmp;
    create_bitmap(&bmp, x, y, 8, 8, 7, 0); // 8x8 font, Light Gray on Black
    
    int row, col;
    int width = bmp.width;
    int height = bmp.height;
    
    unsigned char* map = font_8x8 + (int) c * 8;
    
    int mask[8] = {128, 64, 32, 16, 8, 4, 2, 1};

    for(row = 0; row < height; row++) {
        for(col = 0; col < width; col++) {
            if((map[row] & mask[col]))
                write_pixel8(x + col, y + row, bmp.fore_color);
        }
    }
    
}

/*
**
** Name:  draw_array()
**
** This function draws a character at the x an y coordinate as specified
**
** @param map   The byte map to draw
** @param x     The x coordinate
** @param y     The y coordinate
** @param fore  The foreground color
** @param back  The background color
void draw_array(unsigned char* map, int x, int y, char fore, char back) {
    bmp_t bmp;
    create_bitmap(&bmp, x, y, 8, 8, fore, back); // 8x8 font, Light Gray on Black
    
    int row, col;
    int width = bmp.width;
    int height = bmp.height;
    
    int mask[8] = {128, 64, 32, 16, 8, 4, 2, 1};

    for(row = 0; row < height; row++) {
        for(col = 0; col < width; col++) {
            if((map[row] & mask[col]))
                write_pixel8(x + col, y + row, bmp.fore_color);
            else
                write_pixel8(x + col, y + row, bmp.back_color); 
        }
    }
    
}
*/

/**
** Name:  clear_screen()
**
** Helper function that clears the screen via the creation and 
** filling of a bitmap over the location.
**
*/
void clear_screen(){
    // clear screen
    bmp_t bmp;
    create_bitmap(&bmp, 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, 0, 0); 
}
