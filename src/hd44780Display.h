/*****************************************************************************/
/*                                                                           */
/*!    @brief Driver for alphanumeric Display-Controller HD44780 / KS0070b   */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file     hd44780Display.h                                               */
/*! @see      hd44780Display.c                                               */
/*! @author   Ulrich Becker                                                  */
/*! @date     21.01.2007                                                     */
/* Revision:  31.07.2007 lcdScrollDown()                                  UB */
/*            19.02.2017 Adaptation to Linux-Kernel                       UB */
/******************************************************************************
*    This program is free software; you can redistribute it and/or modify     *
*    it under the terms of the GNU General Public License as published by     *
*    the Free Software Foundation; either version 2 of the License, or        *
*    (at your option) any later version.                                      *
*                                                                             *
*    This program is distributed in the hope that it will be useful,          *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*    GNU General Public License for more details.                             *
*                                                                             *
*    You should have received a copy of the GNU General Public License        *
*    along with this program; if not, write to the Free Software              *
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                *
******************************************************************************/
/* $Id: hd44780Display.h,v 1.1.1.1 2010/01/26 14:02:37 uli Exp $ */
#ifndef _HD44780DISPLAY_H
#define _HD44780DISPLAY_H

#ifdef __KERNEL__
 #include "anLcd_drv.h"
#else
 #ifndef _UI2CTERM_H
  #include <lcd_pin.h>
 #endif
 #if !defined( __PGMSPACE_H_ )         &&\
      defined( __HARVARD_ARCH__ )      &&\
     !defined( _LCD_NO_FLASH_READING )
  #include <avr/pgmspace.h>
 #endif
#endif

#ifdef __KERNEL__
   typedef int          LCD_ADDR_T;
   typedef size_t       LCD_CHAR_SIZE_T;
   typedef size_t       LCD_INDEX_T;
   typedef unsigned int TERMINAL_T;
#else
   typedef signed char  LCD_ADDR_T;
   typedef unsigned char BYTE;
   typedef BYTE         LCD_CHAR_SIZE_T;
   typedef BYTE         LCD_INDEX_T;
   typedef BYTE         TERMINAL_T;
   typedef BYTE         LCD_CGRAM_CHAR_T[8];
   #ifndef ARRAY_SIZE
      #define ARRAY_SIZE( a ) (sizeof(a) / sizeof(a[0]))
   #endif

 #ifndef BIN2BYTE
   #define BIN2BYTE( a ) \
     ((unsigned char) \
     ( ((0x##a##UL >> 21) & 0x80) \
     | ((0x##a##UL >> 18) & 0x40) \
     | ((0x##a##UL >> 15) & 0x20) \
     | ((0x##a##UL >> 12) & 0x10) \
     | ((0x##a##UL >>  9) & 0x08) \
     | ((0x##a##UL >>  6) & 0x04) \
     | ((0x##a##UL >>  3) & 0x02) \
     | ( 0x##a##UL        & 0x01)))
 #endif
 #ifndef SET_PATTERN
    #define SET_PATTERN( p ) BIN2BYTE( 000##p )
 #endif
#endif

#ifndef CONFIG_LCD_MINIMAL

extern BOOL mg_lcdAutoScroll;
#endif
extern BYTE mg_displayState;

#if !defined( LCD_MAX_X ) && !defined( LCD_MAX_Y ) && !defined(__KERNEL__)
  #if defined( LCD_DEM08171 ) || defined( LCD_DEM08172 )
    #define LCD_MAX_X   8
    #define LCD_MAX_Y   1
  #elif defined( LCD_DEM16101 ) || defined( LCD_DEM16102 ) ||\
        defined( LCD_DISPLAYTECH_161 )
    #define LCD_MAX_X  16
    #define LCD_MAX_Y   1
  #elif defined( LCD_DEM16211 ) || defined( LCD_DEM16212 ) ||\
        defined( LCD_DEM16213 ) || defined( LCD_DEM16214 ) ||\
        defined( LCD_DEM16215 ) || defined( LCD_DEM16216 ) ||\
        defined( LCD_DEM16217 ) || defined( LCD_DEM16219 ) ||\
        defined( LCD_DISPLAYTECH_162 )
    #define LCD_MAX_X  16
    #define LCD_MAX_Y   2
  #elif defined( LCD_DISPLAYTECH_164 )
    #define LCD_MAX_X  16
    #define LCD_MAX_Y   4
  #elif defined( LCD_DEM20121 )
    #define LCD_MAX_X  20
    #define LCD_MAX_Y   1
  #elif defined( LCD_DEM20231 ) || defined( LCD_DEM20232 ) ||\
        defined( LCD_DISPLAYTECH_202 )
    #define LCD_MAX_X  20
    #define LCD_MAX_Y   2
  #elif defined( LCD_DEM20485 ) || defined( LCD_DEM20486 ) ||\
        defined( LCD_DISPLAYTECH_204 )
    #define LCD_MAX_X  20
    #define LCD_MAX_Y   4
  #elif defined( LCD_DEM24251 )
    #define LCD_MAX_X  24
    #define LCD_MAX_Y   2
  #elif defined( LCD_DEM40271 )
    #define LCD_MAX_X  40
    #define LCD_MAX_Y   2
  #else
    #warning LCD-type is unknown!
  #endif
#endif

#ifndef LCD_MAX_X
 #define LCD_MAX_X   16
#endif
#ifndef LCD_MAX_Y
 #define LCD_MAX_Y   2
#endif

#ifdef __KERNEL__
   #define MAX_X() global.oLcd.maxX
   #define MAX_Y() global.oLcd.maxY
#else
   #define MAX_X() LCD_MAX_X
   #define MAX_Y() LCD_MAX_Y
#endif


#define LCD_FLAG_CMD_ON_OFF_CTRL  0x08
#define LCD_FLAG_DISPLAY_ON       0x04
#define LCD_FLAG_CURSOR_ON        0x02
#define LCD_FLAG_BLINK_ON         0x01

#define LCD_MAX_CG_RAM 0x07

#define LCD_CG_RAM_UMLAUT_OFFSET (LCD_MAX_CG_RAM - 4)
   
#ifndef CONFIG_LCD_NO_UMLAUT
 #ifdef CONFIG_ST7066U
    #ifndef LCD_UMLAUT_POSITION_Ae
      #define LCD_UMLAUT_POSITION_Ae (LCD_CG_RAM_UMLAUT_OFFSET + 0)
    #endif
    #ifndef LCD_UMLAUT_POSITION_Oe
      #define LCD_UMLAUT_POSITION_Oe (LCD_CG_RAM_UMLAUT_OFFSET + 1)
    #endif
    #ifndef LCD_UMLAUT_POSITION_Ue
      #define LCD_UMLAUT_POSITION_Ue (LCD_CG_RAM_UMLAUT_OFFSET + 2)
    #endif
 #else
    #define LCD_UMLAUT_POSITION_Ae 0xC4
    #define LCD_UMLAUT_POSITION_Oe 0xC6
    #define LCD_UMLAUT_POSITION_Ue 0xCC
 #endif
 #define LCD_UMLAUT_POSITION_ae  0xE1
 #define LCD_UMLAUT_POSITION_oe  0xEF
 #define LCD_UMLAUT_POSITION_ue  0xF5
 #define LCD_UMLAUT_POSITION_ss  0xE2
#endif /* ifndef CONFIG_LCD_NO_UMLAUT */

#ifdef CONFIG_LCD_USE_BACKSLASH
 #define  LCD_BACKSLASH_POSITION   (LCD_CG_RAM_UMLAUT_OFFSET + 3)
#endif
#ifdef CONFIG_LCD_USE_PARAGRAPH
 #define LCD_PARAGRAPH_POSITION    (LCD_CG_RAM_UMLAUT_OFFSET + 4)
#endif

#define LCD_DEGREE_POSITION 0xDF
        
#ifdef __GNUC__
  #define LCD_ATTR_ALWAYS_INLINE __attribute__((always_inline))
#else
  #define LCD_ATTR_ALWAYS_INLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif
BOOL lcdIsBusy( LCD_ADDR_T* pAddr );
LCD_ADDR_T lcdInit( void );
LCD_ADDR_T lcdOn( void );
LCD_ADDR_T lcdOff( void );
LCD_ADDR_T lcdCursorOn( void );
LCD_ADDR_T lcdCursorOff( void );
LCD_ADDR_T lcdCursorBlink( void );
LCD_ADDR_T lcdGotoXY( BYTE x, BYTE y );
LCD_ADDR_T lcdClrscr( void );
LCD_ADDR_T lcdDelLine( BYTE x );
#if LCD_MAX_Y > 1
LCD_ADDR_T lcdScrollUp( void );
LCD_ADDR_T lcdScrollDown( void );
char lcdGetChar( void );
#endif
LCD_ADDR_T lcdLine2Addr( BYTE y );
LCD_ADDR_T lcdSetAddress( LCD_ADDR_T addr );
LCD_ADDR_T lcdPutch( char c );
LCD_CHAR_SIZE_T lcdPuts( const char* str );
static inline BYTE lcdGetDisplayState( void ) { return mg_displayState; }
#if defined( __HARVARD_ARCH__ ) && !defined( CONFIG_LCD_NO_FLASH_READING )
LCD_CHAR_SIZE_T _lcdPutsP( PGM_P strP );
#endif
#ifndef CONFIG_LCD_MINIMAL
LCD_ADDR_T lcdLoadExtraCharP( const LCD_CGRAM_CHAR_T extraChar, BYTE position );
static inline void lcdEnableAutoScroll( void )  LCD_ATTR_ALWAYS_INLINE;
static inline void lcdDisableAutoScroll( void ) LCD_ATTR_ALWAYS_INLINE;
static inline BOOL lcdIsAutoScroll( void )      LCD_ATTR_ALWAYS_INLINE;
#endif
#ifdef LCD_POLL_FUNCTION
void LCD_POLL_FUNCTION ( void );
#endif
#ifdef LCD_BELL_FUNCTION
void LCD_BELL_FUNCTION ( void );
#endif
#ifdef __cplusplus
}
#endif

#if defined( __HARVARD_ARCH__ ) && !defined( _LCD_NO_FLASH_READING )
 #define lcdPutsP( str ) _lcdPutsP( PSTR( str ) )
#else
 #define lcdPutsP( str )  lcdPuts( str )
 #define _lcdPutsP( str ) lcdPuts( str )
#endif
#if LCD_MAX_Y == 1
 #define lcdScrollUp()   lcdDelLine( 1 )
 #define lcdScrollDown() lcdDelLine( 1 )
#endif

#ifndef CONFIG_LCD_MINIMAL
#ifdef __KERNEL__
  void lcdEnableAutoScroll( void )  { global.oLcd.autoScroll = TRUE; }
  void lcdDisableAutoScroll( void ) { global.oLcd.autoScroll = FALSE; }
  BOOL lcdIsAutoScroll( void )      { return global.oLcd.autoScroll; }
#else
  void lcdEnableAutoScroll( void )  { mg_lcdAutoScroll = TRUE; }
  void lcdDisableAutoScroll( void ) { mg_lcdAutoScroll = FALSE; }
  BOOL lcdIsAutoScroll( void )      { return mg_lcdAutoScroll; }
#endif
#ifdef CONFIG_LCD_UNICODE
 #define LCD_UMLAUT_INTRUDUCHER       0xC3
 #define LCD_SPECIAL_CHAR_INTRUDUCHER 0xC2
 BOOL lcdConvertBackUmlaut( char* pChar );
 BOOL lcdConvertBackSpecialChar( char* pChar );
#endif /* ifdef CONFIG_LCD_UNICODE */

#endif /* ifndef CONFIG_LCD_MINIMAL */

#if defined( CONFIG_LCD_USE_BACKSLASH ) && !defined( _CONVERT_CHAR8 )
#define _CONVERT_CHAR8
#endif

#if defined( CONFIG_AN_LCD_READBACK ) && defined( _CONVERT_CHAR8 )
 BOOL lcdConvertBackChar8( char* pChar );
#endif
 void lcdLoadPredefinedExtraCharacters( void );
#endif /* ifndef _HD44780DISPLAY_H */
/*================================== EOF ====================================*/
