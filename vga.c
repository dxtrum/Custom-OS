/**
** @file vga.c
**
** @author CSCI-452 class of 20215
** @author Chris Barnes
**
** This file contains all of the code to enable graphics mode and
** the basic stuff required to interact with the VGA memory location.
** 
** Our operating system will use a 256 color mode with a 320 by 200 pixel
** display
*/

#define	SP_KERNEL_SRC

#include "common.h"
#include "vga.h"
#include "font.h"
#include "bitmap.h"
#include "draw.h"

/*
** PRIVATE DEFINITIONS
*/

//
// VGA Register Port Constants
// 
#define VGA_AC_INDEX    0x3C0
#define VGA_AC_WRITE    0x3C0
#define VGA_AC_READ     0x3C1

#define VGA_MISC        0x3C2

#define VGA_SEQ_INDEX   0x3C4
#define VGA_SEQ_DATA    0x3C5

#define VGA_GC_INDEX    0x3CE
#define VGA_GC_DATA     0x3CF

#define VGA_CRTC_INDEX  0x3D4
#define VGA_CRTC_DATA   0x3D5
#define VGA_AC_RESET    0x3DA

// The following function translation definition was taken
// from the i386 portion WATCOM C defined by Chris Giese <geezer@execpc.com>
// src: https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c 
//
// Convert pokeb calls inta a derefrence that sets the value of a word in VGA
// Memory
#define pokeb(S, O, V)          *(unsigned char *)(16uL * (S) + (O)) = (V)

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
** Name:  _write_registers
**
** This function takes in a series of registers from a byte array
** that will be used to set VGA into graphics mode.
**
** This function was created by Chris Giese and was found at
** the following source:
** https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
** It was slightly modified to make sure the various calls would
** properly run within our system.
**
** @param registers A byte array of the different values that the
** program needs to set at various port values to enable graphics mode
** for the operating system.
*/
void _write_registers(uint8_t* registers) {

    // misc
    __outb(VGA_MISC, *(registers++));

    // seq
    for(uint8_t i = 0; i < 5; i++){
        __outb(VGA_SEQ_INDEX, i);
        __outb(VGA_SEQ_DATA, *(registers++));
    }
    
    // Unlock CRTC
    __outb(VGA_CRTC_INDEX, 0x03);
    __outb(VGA_CRTC_DATA, __inb(VGA_CRTC_DATA) | 0x80);
    
    __outb(VGA_CRTC_INDEX, 0x11);
    __outb(VGA_CRTC_DATA, __inb(VGA_CRTC_DATA) & ~0x80);
    
    // make sure that it remains that way
    registers[0x03] = registers[0x03] | 0x80;
    registers[0x11] = registers[0x11] & ~0x80;

    // crtc 
    for(uint8_t i = 0; i < 25; i++){
        __outb(VGA_CRTC_INDEX, i);
        __outb(VGA_CRTC_DATA, *(registers++));
    }

    // gc
    for(uint8_t i = 0; i < 9; i++){
        __outb(VGA_GC_INDEX, i);
        __outb(VGA_GC_DATA, *(registers++));
    }

    // ac
    for(uint8_t i = 0; i < 21; i++){
        __inb(VGA_AC_RESET);
        __outb(VGA_AC_INDEX, i);
        __outb(VGA_AC_WRITE, *(registers++));
    }

    // lock CRTC and enable the display
    __inb(VGA_AC_RESET);
    __outb(VGA_AC_INDEX, 0x20);
}

/**
** Name:  get_fb_segment
**
** This function returns the current location of the start of the VGA
** Memory based on the correct Graphics Controller Register (0x06)
**
** This function is similar to one created by Chris Giese, but once
** again modified to work for our system and uses a slightly differnt 
** set of bit manipulations to reduce the number of lines needed
**
** @return The VGA mememory start address
*/
unsigned get_fb_segment(void) {
    
    unsigned seg;

    __outb(VGA_GC_INDEX, 0x06);

    // Mask to get the host memory address region
    uint8_t segmentSelector = __inb(VGA_GC_DATA) & (3<<2);
    
    switch(segmentSelector) {
        case 0<<2:
        case 1<<2: seg = 0xA000; break;
        case 2<<2: seg = 0xB000; break;
        case 3<<2: seg = 0xB800; break;
    }

    return seg;
}

/**
** Name: vpokeb
**
** This function is a wrapper for the pokeb function that
** calls get_fb_segment() to make sure that the pokeb will
** start looking into the correct part of VGA memory.
**
** It will then poke the byte at the location into the new value of
** val. THis wrapper was taken from Chris Giese at the following source:
** https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
**
** @param off   The offset into video memory where the correct byte
**              is stored
** @ param val  This is the value to change the effected byte to
*/
void vpokeb(unsigned off, unsigned val) {
    pokeb(get_fb_segment(), off, val);
}


/**
** Name:  _get_color_index
**
** Returns the index of the specified color. Currently not very useful,
** but the idea was to eventually tansform this function into one where
** the proper DAC calls would occur to allow for the full useage of
** 256 color mode
**
** @param color The color to get the index of
**
** @return the index of the color
*/
uint16_t _get_color_index(uint8_t color) {
    uint16_t index;
    
    switch(color) {
        case 0: index = 0x00; // Black
                break;  
        case 1: index = 0x01; // Blue
                break;
        case 2: index = 0x02; // Green
                break;
        case 3: index = 0x03; // Cyan
                break;
        case 4: index = 0x04; // Red
                break;
        case 5: index = 0x05; // Magenta
                break;
        case 6: index = 0x06; // Brown
                break;
        case 7: index = 0x07; // Light Gray
                break; 
        case 8: index = 0x08; // Dark Gray
                break; 
        case 9: index = 0x09; // Light Blue
                break; 
        case 10: index = 0x0A; // Light Green
                break; 
        case 11: index = 0x0B; // Light Cyan
                break; 
        case 12: index = 0x0C; // Light Red
                break; 
        case 13: index = 0x0D; // Light Magenta
                break; 
        case 14: index = 0x0E; // Yelow
                break; 
        case 15: index = 0x0F; // White
                break;
    }
    
    return index;
}

/**
** Name:  write_pixel
**
** Writes a pixel of color c to location (x, y) on the screen.
**
** Taken from Chris Giese at the following location:
** https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
**
** @param x     The x position on the screen (horizontal position)
**              of where to write the pixel.
** @param y     The y position on the screen (vertical position)
**              of where to write the pixel.
** @param c     The new color of the affected pixel
*/
void write_pixel8(unsigned x, unsigned y, unsigned c) {
    unsigned wd_in_bytes;
    unsigned off;

    wd_in_bytes = 320;
    off = wd_in_bytes * y + x;
    vpokeb(off, c);
}

/*
** PUBLIC FUNCTIONS
*/

/**
** Name:  put_pixel
**
** A wrapper for the write_pixel8 method
**
** @param x     The x position on the screen (horizontal position)
**              of where to write the pixel.
** @param y     The y position on the screen (vertical position)
**              of where to write the pixel.
** @param c     The new color of the affected pixel
*/
void put_pixel(uint32_t x, uint32_t y, uint16_t color) {
    write_pixel8(x, y, color);
}

/**
** Name:  _vga_init()
**
** The initialization function for the vga driver. This function
** moves the OS into graphics mode and begins to add basic elements
** onto the screen for the user to see.
**
*/
void _vga_init(void) {
    __cio_puts(" VGA: ");

    _write_registers(g_320x200x256);

    //clears the screen before drawing some basic elements
    clear_screen();
    draw_gui();
}
