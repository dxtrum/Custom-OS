/**
** @file vga.h
**
** @author CSCI-452 class of 20215
** @author Chris Barnes
**
** VGA module declarations
*/

#ifndef VGA_H_
#define VGA_H_

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

// Basic Screen Pixel Dimesion and color depth constants
#define DEFAULT_WIDTH   320
#define DEFAULT_HEIGHT  200
#define DEFAULT_COLOR_DEPTH 256

/*
** Prototypes
*/

//
// _vga_init()
//
// This Function intiates the VGA driver and enables graphics mode
//
void _vga_init(void);

//
// get_fb_buffer()
//
// This function returns the current value of the VGA frame buffer
//
// @return the frame buffer value
unsigned get_fb_segment(void);

// put_pixel()
//
// This function plots a pixel on the screen. A 320x200 screen.
//
// @param x     the x coordinate
// @param y     the y coordinate
// @param color the color of the pixel to plot
void put_pixel(uint32_t x, uint32_t y, uint16_t color);


#endif
/* SP_ASM_SRC */

#endif
