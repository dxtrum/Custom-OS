/**
** @file font.h
**
** @author CSCI-452 class of 20215
**
** The font declarations
*/

#ifndef FONT_H_
#define FONT_H_

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

// The grapics register settings array
unsigned char g_320x200x256[61];

// The main font array
uint8_t font_8x8[2048];

/*
** Prototypes
*/

/**
** Name:  select_font
**
** Choose your font!
**
** @param font	The number describing the font to use
*/
void select_font(int font);

#endif
/* SP_ASM_SRC */

#endif
