/**
** @file ?
**
** @author CSCI-452 class of 20215
**
** ?
*/

#define	SP_KERNEL_SRC

#include "common.h"
#include "vga.h"
#include "draw.h"
#include "bitmap.h"

/*
** PRIVATE DEFINITIONS
*/

// GUI location defintions so that it does not get overwritten by text
#define GUI_MIN_X   0
#define GUI_MAX_X   320
#define GUI_MIN_Y   146
#define GUI_MAX_Y   200

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


/**
** Name:  abs
**
** An absolute value function
**
** @param value     The value to get the absolute value of
**
** @return the absolute value
*/
int abs(int value) {
    if(value < 0) value *= -1;
    return value;
}

/*
** PUBLIC FUNCTIONS
*/

/**
** Name:  draw_line
**
** An implementation of the Bresenham's line algorithm
**
** @param x0        the first x coordinate
** @param x1        the second x coordinate
** @param y0        the first y coordinate
** @param y1        the second y coordinate
** @param color     the color of the line
**
** @return ?
*/
void draw_line(int x0, int x1, int y0, int y1, unsigned color) {
    int dx = abs(x1 - x0);
    int sx = ((x0 < x1) ? 1 : -1);
    int dy = -1 * abs(y1 - y0);
    int sy = ((x0 < x1) ? 1 : -1);
    int error = dx + dy;

    while(true) {
        put_pixel((unsigned) x0, (unsigned) y0, color);
        
        if(x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * error;
        
        if(e2 >= dy) {
            if(x0 == x1) break;
            error = error + dy;
            x0 = x0 + sx;
        }
        
        if(e2 <= dx) {
            if(y0 == y1) break;
            error = error + dx;
            y0 = y0 + sy;
        }
    }

}


/**
** Name:  draw_rect
**
** draw a rectangle onto the screen by creating and filling a bitmap
**
** @param x
**
** @return the bitmap used to draw the rectangle
*/
bmp_t* draw_rect(int x, int y, int width, int height, unsigned fore,
        unsigned back) {
    bmp_t bmp;
    create_bitmap(&bmp, x, y, width, height, fore, back);
    return &bmp;
}

/**
** Name:  draw_gui
**
** Draw the current gui onto the screen
**
*/
void draw_gui(void) {
    bmp_t* header = draw_rect(GUI_MIN_X, GUI_MIN_Y, DEFAULT_WIDTH, 16, 7, 9);
    //bmp_t* close = draw_rect(GUI_MAX_X - 32, GUI_MIN_Y, 32, 16, 8, 0x4);
    //draw_line(GUI_MAX_X - 24, GUI_MAX_X - 8, GUI_MIN_Y + 4, GUI_MIN_Y + 12, 8); 
    //draw_line(GUI_MAX_X - 8, GUI_MAX_X - 24, GUI_MIN_Y + 12, GUI_MIN_Y + 4, 8); 
    bmp_t* body = draw_rect(GUI_MIN_X, GUI_MIN_Y + 16, DEFAULT_WIDTH, 50, 7, 0xA);
}
