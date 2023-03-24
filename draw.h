/**
** @file draw.h
**
** @author CSCI-452 class of 20215
**
** The draw declarations
*/

#ifndef DRAW_H_
#define DRAW_H_

#include "bitmap.h"

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

/*
** Globals
*/

/*
** Prototypes
*/

//
// draw_line()
//
// draw a line to the screen
//
// @param x0        the first x coordinate
// @param x1        the second x coordinate
// @param y0        the first y coordinate
// @param y1        the second y coordinate
// @param color     the color of the line
//
void draw_line(int x0, int x1, int y0, int y1, unsigned color);

//
// draw_rect
//
// Draw a rectangle to the scrren and save its information in a bitmap
//
// @param x         the x coordinate
// @param y         the y coordinate
// @param width     the width d the rectangle
// @param height    the height of the rectangle
// @param fore      the foreground color bitmap
// @param back      the background color of the bitmap
//                  and the color of the rectangle
bmp_t* draw_rect(int x, int y, int width, int height, unsigned fore,
        unsigned back);
//
// draw_gui()
//
// draw the defined GUI to the screen
void draw_gui(void);

#endif
/* SP_ASM_SRC */

#endif
