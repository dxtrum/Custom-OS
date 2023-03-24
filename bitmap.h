/**
** @file bitmap.h
**
** @author CSCI-452 class of 20215
**
** The bitmap delarations file
*/

#ifndef BITMAP_H_
#define BITMAP_H_

/*
** General (C and/or assembly) definitions
**
** This section of the header file contains definitions that can be
** used in either C or assembly-language source code.
*/

#ifndef SP_ASM_SRC

/*
** Start of C-only definitions
**
** Anything that should not be visible to something other than
** the C compiler should be put here.
*/

/*
** Types
*/

// A bitmap structure
typedef struct bitmap {
    uint16_t width;         // width of the bitmap
    uint16_t height;        // height of the bitmap
    unsigned char* raster;  // pointer to the begining of video memory
    unsigned fore_color;    // the foreground color
    unsigned back_color;    // the background color
} bmp_t;

/*
** Globals
*/

/*
** Prototypes
*/

//
// clear_screen()
//
// clears the screen entirely
//
void clear_screen(void);

//
// draw_character()
//
// Draw a character to a given x and y coordinate
//
// @param x     the x coordinate
// @param y     the y coordinate
// @param c     the color of the character
//
void draw_character(int x, int y, char c);

/*
//
// draw_array()
//
// Draw an array to a given x and y coordinate
//
// @param map   the byte array to draw
// @param x     the x coordinate
// @param y     the y coordinate
// @param fore  the foreground color
// @param back  the background color
//
void draw_array(unsigned char* map, int x, int y, char fore, char back);
*/

//
// create_bitmap()
//
// Create and fill a new bitmap starting at a particular location
// 
// @param x             the x coordinate
// @param y             the y coordinate
// @param width         the width of the bitmap
// @param height        the height of the bitmap
// @param fore_color    the forground color
// @param back_color    the background color
//
void create_bitmap(bmp_t* bmp, int x, int y, int width, int height,
        unsigned fore_color, unsigned back_color);

//
// create_bitmap_safe()
//
// Create a new bitmap starting at a particular location
// 
// @param x             the x coordinate
// @param y             the y coordinate
// @param width         the width of the bitmap
// @param height        the height of the bitmap
// @param fore_color    the forground color
// @param back_color    the background color
void create_bitmap_safe(bmp_t* bmp, int x, int y, int width, int height,
        unsigned fore_color, unsigned back_color);

//
// blit()
//
// Copy the contents of one bitmap into another bitmap
//
// @param src   the source bitmap
// @param dest  the destination bitmap
//
void blit(bmp_t* src, bmp_t* dest);

// 
// draw_bitmap
//
// draw a bitmap at a particulat location
//
// @param bmp       The bitmap to draw
// @param x         The x coordinate
// @param y         The Y coordinate
//
void draw_bitmap(bmp_t* bmp, int x, int y);

#endif
/* SP_ASM_SRC */

#endif
