/*****************************************************************************/
/*                                                                           */
/*!  @brief Driver for alphanumeric Display-Controller HD44780 / KS0070b     */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file     hd44780Display.c                                               */
/*! @see      hd44780Display.h                                               */
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
/* $Id: hd44780Display.c,v 1.1.1.1 2010/01/26 14:02:37 uli Exp $ */
#ifndef __KERNEL__
  #include <avr/io.h>
  #include <util/delay.h>
  #include <utermctrl.h>
  #include <u_assert.h>
#endif
#include "hd44780Display.h"

#ifndef CONFIG_TLCD_CLK_DELAY
  #define CONFIG_TLCD_CLK_DELAY 50 //!<@brief CLK-delay in microseconds.
#endif

#ifndef __KERNEL__
  #ifndef LCD_CONTROL_PORT
    #error LCD_CONTROL_PORT is not defined!
  #endif
  #ifndef LCD_CONTROL_DDR
    #error LCD_CONTROL_DDR is not defined!
  #endif
  #ifndef LCD_RS_MASK
    #error LCD_RS_MASK (pin-mask of RS) is not defined!
  #endif
  #ifndef LCD_RW_MASK
    #error LCD_RW_MASK (pin-mask of RW) is not defined!
  #endif
  #ifndef LCD_EN_MASK
    #error LCD_EN_MASK (pin-mask of EN) is not defined!
  #endif
  #ifndef LCD_DATA_PORT
    #error LCD_DATA_PORT is not defined!
  #endif
  #ifndef LCD_DATA_DDR
    #error LCD_DATA_DDR is not defined!
  #endif
  #ifndef LCD_DATA_PIN
    #error LCD_DATA_PIN is not defined!
  #endif
#endif /* ifndef __KERNEL__ */

#ifdef __KERNEL__
  #ifdef _LCD_DATAPORT_PINS_8
    #error Macro _LCD_DATAPORT_PINS_8 is defined for Linux-kernel \
           but it sopported 4-bit mode only!
  #endif
  #define LCD_DATA_MASK 0x0F
  #define LCD_PORT_SHIFT 0
#else
  #ifndef _LCD_DATAPORT_PINS_8
   #if LCD_DATA_MASK == 0x0F
     #define LCD_PORT_SHIFT 0
   #elif LCD_DATA_MASK == 0x1E
     #define LCD_PORT_SHIFT 1
   #elif LCD_DATA_MASK == 0x3C
     #define LCD_PORT_SHIFT 2
   #elif LCD_DATA_MASK == 0x78
     #define LCD_PORT_SHIFT 3
   #elif LCD_DATA_MASK == 0xF0
     #define LCD_PORT_SHIFT 4
   #else
     #error LCD_DATA_MASK is invalid or not defined!
   #endif
  #else
   #ifdef LCD_DATA_MASK
     #undef LCD_DATA_MASK
   #endif
   #define LCD_DATA_MASK 0xFF
  #endif
#endif

#ifdef __KERNEL__
  #define LCD_SET_EN_LOW()         gpio_direction_output( global.oLcd.port.en.number, 0 )
  #define LCD_SET_EN_HIGH()        gpio_direction_input( global.oLcd.port.en.number )
  #define LCD_SET_HIGH_IMPEDANCE() writeLcdPort( 0xFF );
  #define LCD_SELECT_INSTRUCTION() gpio_direction_output( global.oLcd.port.rs.number, 0 )
  #define LCD_SELECT_DATA()        gpio_direction_input( global.oLcd.port.rs.number )
  #define LCD_MODE_READ()          gpio_direction_input( global.oLcd.port.rw.number )
  #define LCD_MODE_WRITE()         gpio_direction_output( global.oLcd.port.rw.number, 0 )
  #define LCD_INIT_WAIT()          mdelay( 200 )
  #define LCD_CLK_WAIT()           udelay( CONFIG_TLCD_CLK_DELAY )
  #define LCD_POLL_FUNCTION()      schedule()
  #define LCD_ERROR_HANDLING()     global.oLcd.isInitialized = false
#else /* AVR */
  #define LCD_SET_EN_LOW()         LCD_CONTROL_PORT &= ~LCD_EN_MASK
  #define LCD_SET_EN_HIGH()        LCD_CONTROL_PORT |= LCD_EN_MASK

  #ifdef _LCD_DDR_INVERS
    #define LCD_SET_HIGH_IMPEDANCE() LCD_DATA_DDR |= LCD_DATA_MASK
  #else
    #define LCD_SET_HIGH_IMPEDANCE() LCD_DATA_DDR &= ~LCD_DATA_MASK
  #endif

  #define LCD_SELECT_INSTRUCTION() LCD_CONTROL_PORT &= ~LCD_RS_MASK
  #define LCD_SELECT_DATA()        LCD_CONTROL_PORT |= LCD_RS_MASK

  #define LCD_MODE_READ()          LCD_CONTROL_PORT |= LCD_RW_MASK
  #define LCD_MODE_WRITE()         LCD_CONTROL_PORT &= ~LCD_RW_MASK

  #ifndef LCD_CLK_WAIT
    #define LCD_CLK_WAIT()  _delay_us( 30.0 )
  #endif
  #ifndef LCD_INIT_WAIT
    #define LCD_INIT_WAIT() _delay_ms( 200.0 )
  #endif
  #ifndef LCD_POLL_FUNCTION
     #define LCD_POLL_FUNCTION()
  #endif
  #ifndef LCD_ERROR_HANDLING
    #define LCD_ERROR_HANDLING()
  #endif
  #ifndef ERROR_MESSAGE
    #define ERROR_MESSAGE()
  #endif
  #ifndef DEBUG_MESSAGE
    #define DEBUG_MESSAGE()
  #endif
#endif /* else of ifdef __KERNEL__ */


#define LCD_MEM_LINE_SIZE        0x40

#ifndef __KERNEL__
 #if LCD_MAX_Y == 1
   #define LCD_MAX_ADDR (LCD_MAX_X -1)
 #elif  LCD_MAX_Y == 2
   #define LCD_MAX_ADDR (LCD_MEM_LINE_SIZE + LCD_MAX_X - 1)
 #elif  LCD_MAX_Y == 4
   #define LCD_MAX_ADDR (LCD_MEM_LINE_SIZE + LCD_MAX_X + LCD_MAX_X - 1)
 #else
   #error LCD_MAX_Y is invalid!
 #endif
 #define readLcdPort()     LCD_DATA_PIN
 #define writeLcdPort( b ) LCD_DATA_DDR |= (b)
#endif

#ifdef __HARVARD_ARCH__
  #if (FLASHEND > 0xFFFFUL) && defined(_LCD_USE_FAR_FLASH )
   #if defined( __GNUC__ ) && defined( __AVR__ )
     #error Sorry, but AVR-GCC do not support far-pointer yet!
   #endif
   #define LCD_PGM_READ_BYTE( p )  pgm_read_byte_far( p )
  #else
   #define LCD_PGM_READ_BYTE( p )  pgm_read_byte_near( p )
  #endif
#else
  #define LCD_PGM_READ_BYTE( p ) *(p)
  #define PGM_P const
#endif

#ifndef CONFIG_TLCD_MAX_POLL
   #define CONFIG_TLCD_MAX_POLL 100
#endif

#ifdef __KERNEL__
  typedef unsigned int LCD_POLL_T;
#else
  #if CONFIG_TLCD_MAX_POLL <= 0xFF
    typedef BYTE LCD_POLL_T;
  #else
    typedef WORD LCD_POLL_T;
  #endif
#endif

#ifndef _LCD_MINIMAL
#ifdef __KERNEL__
 #define mg_lcdAutoScroll global.oLcd.autoScroll
 #define mg_lastChar      global.oLcd.lastChar
#else
 BOOL mg_lcdAutoScroll;
 static BOOL mg_lastChar;
#endif

#ifndef CONFIG_LCD_NO_UMLAUT

FLASH LCD_CGRAM_CHAR_T mg_char_Ae =
{            /* 43210 */
   SET_PATTERN( 01010 ), /*  # #  */
   SET_PATTERN( 01110 ), /*  ###  */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 11111 ), /* ##### */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 00000 )  /*       */
};

LCD_CGRAM_CHAR_T FLASH mg_char_Oe =
{            /* 43210 */
   SET_PATTERN( 01010 ), /*  # #  */
   SET_PATTERN( 01110 ), /*  ###  */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 01110 ), /*  ###  */
   SET_PATTERN( 00000 )  /*       */
};

LCD_CGRAM_CHAR_T FLASH mg_char_Ue =
{            /* 43210 */
   SET_PATTERN( 01010 ), /*  # #  */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 10001 ), /* #   # */
   SET_PATTERN( 01110 ), /*  ###  */
   SET_PATTERN( 00000 )  /*       */
};
#endif /* ifndef CONFIG_LCD_NO_UMLAUT */
#ifdef CONFIG_LCD_USE_BACKSLASH
LCD_CGRAM_CHAR_T FLASH mg_char_backslash =
{
   SET_PATTERN( 00000 ),
   SET_PATTERN( 10000 ),
   SET_PATTERN( 01000 ),
   SET_PATTERN( 00100 ),
   SET_PATTERN( 00010 ),
   SET_PATTERN( 00001 ),
   SET_PATTERN( 00000 ),
   SET_PATTERN( 00000 )
};
#endif /* ifdef CONFIG_LCD_USE_BACKSLASH */
#ifdef CONFIG_LCD_USE_PARAGRAPH
LCD_CGRAM_CHAR_T FLASH mg_char_paragraph =
{
   SET_PATTERN( 00111 ),
   SET_PATTERN( 01000 ),
   SET_PATTERN( 01110 ),
   SET_PATTERN( 10001 ),
   SET_PATTERN( 01110 ),
   SET_PATTERN( 00010 ),
   SET_PATTERN( 11100 ),
   SET_PATTERN( 00000 )
};
#endif /*ifdef CONFIG_LCD_USE_PARAGRAPH*/

#if !(defined( CONFIG_LCD_NO_UMLAUT ) && defined( CONFIG_LCD_NO_TERMINAL ))
typedef struct
{
   BYTE c;
   BYTE d;
} CHAR_MAP_T;

#ifdef _CONVERT_CHAR8
CHAR_MAP_T FLASH mg_char8Map[] =
{
 #ifdef CONFIG_LCD_USE_BACKSLASH
   { '\\', LCD_BACKSLASH_POSITION },
 #endif
 #ifndef CONFIG_LCD_UNICODE
   { 0x84, LCD_UMLAUT_POSITION_ae },
   { 0x8E, LCD_UMLAUT_POSITION_Ae },
   { 0x99, LCD_UMLAUT_POSITION_Oe },
   { 0x94, LCD_UMLAUT_POSITION_oe },
   { 0x81, LCD_UMLAUT_POSITION_ue },
   { 0x9A, LCD_UMLAUT_POSITION_Ue },
   { 0x7E, LCD_UMLAUT_POSITION_ss },
 #endif
   { 0, 0 } // Please don't forget this terminator!
};
#endif /* ifdef _CONVERT_CHAR8 */

CHAR_MAP_T FLASH mg_umlautMap[] =
{
   { 0xA4, LCD_UMLAUT_POSITION_ae }, // ä
   { 0xB6, LCD_UMLAUT_POSITION_oe }, // ö
   { 0xBC, LCD_UMLAUT_POSITION_ue }, // ü
   { 0x9F, LCD_UMLAUT_POSITION_ss }, // ß
   { 0x84, LCD_UMLAUT_POSITION_Ae }, // Ä
   { 0x96, LCD_UMLAUT_POSITION_Oe }, // Ö
   { 0x9C, LCD_UMLAUT_POSITION_Ue }, // Ü
   { 0, 0 }  // Please don't forget this terminator!
};

CHAR_MAP_T FLASH mg_specialCharMap[] =
{
   { 0xB0, LCD_DEGREE_POSITION    }, // °
#ifdef CONFIG_LCD_USE_PARAGRAPH
   { 0xA7, LCD_PARAGRAPH_POSITION }, // §
#endif
   { 0, 0 } // Please don't forget this terminator!
};

typedef enum
{
   LCD_NORMAL,
   LCD_UMLAUT,
   LCD_SPECIAL_CHAR,
   LCD_ESC,
   LCD_CTRL,
   LCD_CURSOR
} LCD_STATUS_T;
LCD_STATUS_T mg_putchState;

#if defined(__KERNEL__) && defined( CONFIG_DEBUG_TLCD_FSM )
/*!---------------------------------------------------------------------------
@brief Function converts the module-states in a human readable ASCII-string
       for debugging purposes.
@see STATE_T
*/
static const char* getStateStr( LCD_STATUS_T state )
{
   #define ST_ENUM_CASE_ITEM( s ) case s: return #s;
   switch( state )
   {
      ST_ENUM_CASE_ITEM( LCD_NORMAL );
      ST_ENUM_CASE_ITEM( LCD_UMLAUT );
      ST_ENUM_CASE_ITEM( LCD_ESC );
      ST_ENUM_CASE_ITEM( LCD_CTRL );
      ST_ENUM_CASE_ITEM( LCD_CURSOR );
      default: BUG_ON( true ); break;
   }
   #undef ST_ENUM_CASE_ITEM
   return "undefined";
}

  #define TRANSITION( newState ) \
  do \
  { \
     if( mg_putchState != newState ) \
        DEBUG_MESSAGE( ": Set state %s -> %s\n", getStateStr( mg_putchState ), #newState ); \
     mg_putchState = newState; \
  } \
  while( false )
#else
  #define TRANSITION( newState ) mg_putchState = newState
#endif

#endif
#endif /* ifndef _LCD_MINIMAL */

#ifdef __KERNEL__
 #define mg_displayState global.oLcd.displayState
#else
 BYTE mg_displayState;
#endif

/*----------------------------- _lcdSetData -----------------------------------
*/
static void _lcdSetData( BYTE d )
{
   LCD_MODE_WRITE();
   LCD_CLK_WAIT();
#if !defined(_LCD_DDR_INVERS ) && !defined(__KERNEL__)
   d = ~d;
#endif
   LCD_SET_EN_HIGH();
   LCD_CLK_WAIT();
#ifdef _LCD_DATAPORT_PINS_8
   writeLcdPort( d );
#else
   //!!LCD_SET_HIGH_IMPEDANCE();
   writeLcdPort( ((d << LCD_PORT_SHIFT) & LCD_DATA_MASK ) );
#endif
   LCD_CLK_WAIT();
   LCD_SET_EN_LOW();
}

/*---------------------------- lcdGetData -------------------------------------
*/
static BYTE lcdGetData( void )
{
   BYTE data;

   LCD_SET_HIGH_IMPEDANCE(); 
   LCD_MODE_READ();
   LCD_CLK_WAIT();
   LCD_SET_EN_HIGH();
   LCD_CLK_WAIT();
#ifdef _LCD_DATAPORT_PINS_8
   data = readLcdPort();
#else
   data = ((readLcdPort() & LCD_DATA_MASK) >> LCD_PORT_SHIFT) << 4;
   LCD_SET_EN_LOW();
   LCD_CLK_WAIT();
   LCD_SET_EN_HIGH();
   LCD_CLK_WAIT();
   data |= (readLcdPort() & LCD_DATA_MASK) >> LCD_PORT_SHIFT;
#endif
   LCD_SET_EN_LOW();
   return data;
}

#if (LCD_MAX_Y > 1) || defined(__KERNEL__)
/*------------------------------ lcdGetChar -----------------------------------
*/
char lcdGetChar( void )
{
   LCD_SELECT_DATA();
   return lcdGetData();
}
#endif

/*------------------------------ lcdIsBusy ------------------------------------
*/
BOOL lcdIsBusy( LCD_ADDR_T* pAddr )
{
   LCD_SELECT_INSTRUCTION(); // RS = 0
   LCD_SET_HIGH_IMPEDANCE()
   LCD_CLK_WAIT();
   *pAddr = lcdGetData();
   return (*pAddr & 0x80) != 0;
}

/*----------------------------- lcdSetData ------------------------------------
*/
static LCD_ADDR_T lcdSetData( BYTE d )
{
   LCD_ADDR_T addr;
   LCD_POLL_T poll = CONFIG_TLCD_MAX_POLL;

   LCD_MODE_WRITE();
 #ifndef _LCD_DATAPORT_PINS_8
   _lcdSetData( d >> 4 ); // High
 #endif
   _lcdSetData( d );      // Low
   while( lcdIsBusy( &addr ) )
   {
     // DEBUG_MESSAGE( " %d\n", poll );
      poll--;
      if( poll == 0 )
      {
         ERROR_MESSAGE( ": Unable to set LCD-data %02X\n", d );
         LCD_ERROR_HANDLING();
         break;
      }
      LCD_POLL_FUNCTION();
   }
   return addr;
}

/*----------------------------- lcdSetAddress ---------------------------------
*/
LCD_ADDR_T lcdSetAddress( LCD_ADDR_T addr )
{
   LCD_ADDR_T ret;

   DEBUG_MESSAGE( ": %02X\n", addr );
   LCD_SELECT_INSTRUCTION();
   ret = lcdSetData( addr | 0x80 );
   if( ret != addr )
   {
      ERROR_MESSAGE( ": Unable to set address %02X -> %02X\n", addr, ret );
      LCD_ERROR_HANDLING();
      return -1;
   }
   return ret;
}

/*------------------------------- lcdSetChar ----------------------------------
*/
static
LCD_ADDR_T lcdSetChar( char c )
{
   LCD_SELECT_DATA();
   return lcdSetData( c );
}

#ifndef CONFIG_LCD_MINIMAL
/*------------------------------ lcdSetCgAddress ------------------------------
*/
static INLINE LCD_ADDR_T lcdSetCgAddress( LCD_ADDR_T addr )
{
   LCD_SELECT_INSTRUCTION();
   return lcdSetData( (addr & 0x7F) | 0x40 );
}

/*------------------------------ lcdWriteCgRam --------------------------------
*/
static INLINE LCD_ADDR_T lcdWriteCgRam( LCD_ADDR_T addr, BYTE data )
{
   addr = lcdSetCgAddress( addr );
   if( addr < 0 )
      return addr;
   return lcdSetChar( data );
}

/*---------------------------- lcdLoadExtraCharP ------------------------------
*/
LCD_ADDR_T lcdLoadExtraCharP( const LCD_CGRAM_CHAR_T extraChar, BYTE position )
{
   LCD_INDEX_T i;
   LCD_ADDR_T addr;

   if( position > LCD_MAX_CG_RAM )
   {
      ERROR_MESSAGE( ": %d is out of range of %d\n", position, LCD_MAX_CG_RAM );
      return -1;
   }

   ASSERT( extraChar != NULL );

   position *= sizeof( LCD_CGRAM_CHAR_T );
   for( i = 0; i < sizeof( LCD_CGRAM_CHAR_T ); i++ )
   {
      addr = lcdWriteCgRam( position++, LCD_PGM_READ_BYTE( extraChar++ ) );
      if( addr < 0 )
         break;
   }
   return addr;
}

#endif /* ifndef CONFIG_LCD_MINIMAL */

#if LCD_MAX_Y > 1
/*------------------------------- lcdAddr2Line --------------------------------
*/
BYTE lcdAddr2Line( LCD_ADDR_T addr )
{
   if( addr < MAX_X() )
      return 0;
 #if (LCD_MAX_Y > 2) || defined(__KERNEL__)
  #if LCD_MAX_Y > 3 || defined(__KERNEL__)
   if( addr < LCD_MEM_LINE_SIZE )
      return 2;
   if( addr >= LCD_MEM_LINE_SIZE && addr < (LCD_MEM_LINE_SIZE + MAX_X()) )
      return 1;
   return 3;
  #else
   if( addr < LCD_MEM_LINE_SIZE )
      return 2;
   return 1;
  #endif
 #else
   return 1;
 #endif
}

/*---------------------------- lcdLine2Addr -----------------------------------
*/
LCD_ADDR_T lcdLine2Addr( BYTE y )
{
   switch( y )
   {
      case 1: return LCD_MEM_LINE_SIZE;
    #if (LCD_MAX_Y > 2) || defined(__KERNEL__)
      case 2: return MAX_X();
     #if (LCD_MAX_Y > 3) || defined(__KERNEL__)
      case 3: return MAX_X() + LCD_MEM_LINE_SIZE;
     #endif
    #endif
      default: return 0;
   }
}

/*---------------------------- lcdAddr2LineAddr -------------------------------
*/
static LCD_ADDR_T lcdAddr2LineAddr( LCD_ADDR_T addr )
{
   return lcdLine2Addr( lcdAddr2Line( addr ) );
}

#else
 #define lcdAddr2Line( x )     0
 #define lcdLine2Addr( x )     0
 #define lcdAddr2LineAddr( x ) 0
#endif

/*----------------------------- lcdGotoXY -------------------------------------
*/
LCD_ADDR_T lcdGotoXY( BYTE x, BYTE y )
{
   BOOL error = FALSE;
   DEBUG_MESSAGE( ": x=%d, y=%d\n", x, y );

   if( (x < 1) || (x > MAX_X()) )
   {
      ERROR_MESSAGE( ": Position X out of range: %d\n", x );
      error = TRUE;
   }
   if( (y < 1) || (y > MAX_Y()) )
   {
      ERROR_MESSAGE( ": Position Y out of range: %d\n", y );
      error = TRUE;
   }
   if( error )
      return -1;

#if (LCD_MAX_Y > 2) || defined(__KERNEL__)
   mg_lastChar = ((x == MAX_X()) && (y == MAX_Y()));
#endif
   return lcdSetAddress( (x-1) + lcdLine2Addr( y-1 ) );
}

/*----------------------------- lcdCursorOn -----------------------------------
*/
LCD_ADDR_T lcdCursorOn( void )
{
   DEBUG_MESSAGE( "\n" );
   LCD_SELECT_INSTRUCTION();
   mg_displayState |= LCD_FLAG_CURSOR_ON;
   return lcdSetData( mg_displayState | LCD_FLAG_CMD_ON_OFF_CTRL );
}

/*----------------------------- lcdCursorOff ----------------------------------
*/
LCD_ADDR_T lcdCursorOff( void )
{
   DEBUG_MESSAGE( "\n" );
   LCD_SELECT_INSTRUCTION();
   mg_displayState &= ~(LCD_FLAG_CURSOR_ON | LCD_FLAG_BLINK_ON);
   return lcdSetData( mg_displayState | LCD_FLAG_CMD_ON_OFF_CTRL );
}

/*---------------------------- lcdCursorBlink ---------------------------------
*/
LCD_ADDR_T lcdCursorBlink( void )
{
   DEBUG_MESSAGE( "\n" );
   LCD_SELECT_INSTRUCTION();
   mg_displayState |= LCD_FLAG_BLINK_ON;
   return lcdSetData( mg_displayState | LCD_FLAG_CMD_ON_OFF_CTRL );
}

/*--------------------------------- lcdOn -------------------------------------
*/
LCD_ADDR_T lcdOn( void )
{
   DEBUG_MESSAGE( "\n" );
   LCD_SELECT_INSTRUCTION();
   mg_displayState |= LCD_FLAG_DISPLAY_ON;
   return lcdSetData( mg_displayState | LCD_FLAG_CMD_ON_OFF_CTRL );
}

/*--------------------------------- lcdOff ------------------------------------
*/
LCD_ADDR_T lcdOff( void )
{
   DEBUG_MESSAGE( "\n" );
   LCD_SELECT_INSTRUCTION();
   mg_displayState &= ~LCD_FLAG_DISPLAY_ON;
   return lcdSetData( mg_displayState | LCD_FLAG_CMD_ON_OFF_CTRL );
}

/*--------------------------------- lcdClrscr ---------------------------------
*/
LCD_ADDR_T lcdClrscr( void )
{
   mg_lastChar = FALSE;
   DEBUG_MESSAGE( "\n" );
   LCD_SELECT_INSTRUCTION();
   return lcdSetData( 0x01 );
}

/*-------------------------------- lcdDelLine ---------------------------------
*/
LCD_ADDR_T lcdDelLine( BYTE x )
{
   LCD_ADDR_T addr, i;

   DEBUG_MESSAGE( ": %d\n", x );

   if( (x < 1) || (x > MAX_X()) )
   {
      ERROR_MESSAGE( ": Position X out of range: %d\n", x );
      return -1;
   }

   LCD_SELECT_INSTRUCTION();
   if( lcdIsBusy( &addr ) )
      return addr;

   x--;
   addr = lcdSetAddress( x + lcdAddr2LineAddr( addr ) );
   if( addr < 0 )
      return addr;
   for( i = x; i < MAX_X(); i++ )
   {
      addr = lcdSetChar( ' ' );
      if( addr < 0 )
         return addr;
   }
 #if (LCD_MAX_Y > 2) || defined(__KERNEL__)
   mg_lastChar = (x == MAX_X()-1);
   if( addr == 0 )
      return lcdSetAddress( LCD_MEM_LINE_SIZE + MAX_X() );
 #endif
   return lcdSetAddress( addr + x - MAX_X() );
}

#if (LCD_MAX_Y > 1) || defined(__KERNEL__)

#if LCD_MAX_Y == 2 && !defined(__KERNEL__)
   #define o1 LCD_MEM_LINE_SIZE
   #define o2 0
#endif

/*-------------------------------- lcdScrollUp --------------------------------
*/
LCD_ADDR_T lcdScrollUp( void )
{
   LCD_ADDR_T addr, x;
   BYTE data[MAX_X()];
#if (LCD_MAX_Y > 2) || defined(__KERNEL__)
   LCD_INDEX_T y;
   LCD_ADDR_T o1, o2;

   DEBUG_MESSAGE( "\n" );
   for( y = 0; y < (MAX_Y() - 1); y++ )
   {
      o1 = lcdLine2Addr( y + 1 );
      o2 = lcdLine2Addr( y );
#endif
      addr = lcdSetAddress( o1 );
      if( addr < 0 )
         return addr;

      for( x = 0; x < MAX_X(); x++ )
      {
         data[x] = lcdGetChar();
         DEBUG_MESSAGE( ": %02X %c\n", data[x], data[x] );
      }

      addr = lcdSetAddress( o2 );
      if( addr < 0 )
         return addr;

      for( x = 0; x < MAX_X(); x++ )
      {
         addr = lcdSetChar( data[x] );
         if( addr < 0 )
            return addr;
      }
#if (LCD_MAX_Y > 2) || defined(__KERNEL__)
   }
#endif
   addr = lcdSetAddress( o1 );
   if( addr < 0 )
      return addr;
   return lcdDelLine( 1 );
}

/*---------------------------- lcdScrollDown ----------------------------------
*/
LCD_ADDR_T lcdScrollDown( void )
{
   LCD_ADDR_T addr, x;
   BYTE data[MAX_X()];
#if (LCD_MAX_Y > 2) || defined(__KERNEL__)
   LCD_INDEX_T y;
   LCD_ADDR_T o1, o2;

   DEBUG_MESSAGE( "\n" );
   for( y = MAX_Y()-1; y > 0; y-- )
   {
      o1 = lcdLine2Addr( y );
      o2 = lcdLine2Addr( y - 1 );
#endif
      addr = lcdSetAddress( o2 );
      if( addr < 0 )
         return addr;

      for( x = 0; x < MAX_X(); x++ )
      {
         data[x] = lcdGetChar();
         DEBUG_MESSAGE( ": %02X %c\n", data[x], data[x] );
      }

      addr = lcdSetAddress( o1 );
      if( addr < 0 )
         return addr;

      for( x = 0; x < MAX_X(); x++ )
      {
         addr = lcdSetChar( data[x] );
         if( addr < 0 )
           return addr;
      }

#if (LCD_MAX_Y > 2) || defined(__KERNEL__)
   }
#endif
   addr = lcdSetAddress( 0 );
   if( addr < 0 )
      return addr;
   return lcdDelLine( 1 );
}

#if LCD_MAX_Y == 2 && !defined(__KERNEL__)
   #undef o1
   #undef o2
#endif

#endif /* if LCD_MAX_Y > 1 */

#if defined( CONFIG_LCD_UNICODE ) || defined( _CONVERT_CHAR8 )
/*!----------------------------------------------------------------------------
 */
static BOOL lcdConvert( char* pChar, PGM_P CHAR_MAP_T paCarMap[] )
{
   LCD_INDEX_T i;
   BYTE pattern;

   i = 0;
   while( (pattern = LCD_PGM_READ_BYTE( &paCarMap[i].c )) != 0 )
   {
      if( pattern == *pChar )
      {
         *pChar = LCD_PGM_READ_BYTE( &paCarMap[i].d );
         return TRUE;
      }
      i++;
   }
   return FALSE;
};

#ifdef _CONVERT_CHAR8
static inline BOOL lcdConvertChar8( char* pChar )
{
   return lcdConvert( pChar, mg_char8Map );
}
#endif

#ifndef CONFIG_LCD_NO_UMLAUT
/*!----------------------------------------------------------------------------
 */
static inline BOOL lcdConvertUmlaut( char* pChar )
{
   return lcdConvert( pChar, mg_umlautMap );
}

/*!----------------------------------------------------------------------------
 */
static inline BOOL lcdConvertSpecialChar( char* pChar )
{
   return lcdConvert( pChar, mg_specialCharMap );
}

#ifdef CONFIG_AN_LCD_READBACK
/*!----------------------------------------------------------------------------
 */
static BOOL lcdConvertBack( char* pChar, PGM_P CHAR_MAP_T paCarMap[] )
{
   LCD_INDEX_T i;
   BYTE pattern;

   i = 0;
   while( (pattern = LCD_PGM_READ_BYTE( &paCarMap[i].c )) != 0 )
   {
      if( *pChar == LCD_PGM_READ_BYTE( &paCarMap[i].d ) )
      {
         *pChar = pattern;
         return TRUE;
      }
      i++;
   }
   return FALSE;
}

/*!----------------------------------------------------------------------------
 */
BOOL lcdConvertBackUmlaut( char* pChar )
{
   return lcdConvertBack( pChar, mg_umlautMap );
}

/*!----------------------------------------------------------------------------
 */
BOOL lcdConvertBackSpecialChar( char* pChar )
{
   return lcdConvertBack( pChar, mg_specialCharMap );
}

#ifdef _CONVERT_CHAR8
/*!----------------------------------------------------------------------------
 */
BOOL lcdConvertBackChar8( char* pChar )
{
   return lcdConvertBack( pChar, mg_char8Map );
}
#endif /* ifdef _CONVERT_CHAR8 */
#endif /* ifdef CONFIG_AN_LCD_READBACK */
#endif /* ifndef CONFIG_LCD_NO_UMLAUT */
#endif /* defined( CONFIG_LCD_UNICODE ) || defined( _CONVERT_CHAR8 ) */

/*----------------------------- lcdPutch --------------------------------------
*/
LCD_ADDR_T lcdPutch( char c )
{
#ifndef _LCD_MINIMAL
   LCD_ADDR_T addr;
   BOOL isLastLine;
 #if !(defined( CONFIG_LCD_NO_UMLAUT ) && defined( CONFIG_LCD_NO_TERMINAL ))
  #ifndef CONFIG_LCD_NO_TERMINAL
   static TERMINAL_T  parameter[2];
   static LCD_INDEX_T i = 0;
  #endif
 #endif
   if( lcdIsBusy( &addr ) )
   {
      DEBUG_MESSAGE( ": LCD is still busy!\n" );
      return addr;
   }
 #if !(defined( CONFIG_LCD_NO_UMLAUT ) && defined( CONFIG_LCD_NO_TERMINAL ))
   switch( mg_putchState )
   {
    #if !defined( CONFIG_LCD_NO_UMLAUT ) && defined( CONFIG_LCD_UNICODE )
      case LCD_UMLAUT:
      {
         if( lcdConvertUmlaut( &c ) )
            break;
         ERROR_MESSAGE( ": Character %02X not found in umlaut-map!\n", c );
         TRANSITION( LCD_NORMAL );
         return -1;
      }
      case LCD_SPECIAL_CHAR:
      {
         if( lcdConvertSpecialChar( &c ) )
            break;
         ERROR_MESSAGE( ": Character %02X not found in special-character-map!\n", c );
         TRANSITION( LCD_NORMAL );
         return -1;
      }
    #endif /* if !defined( CONFIG_LCD_NO_UMLAUT ) && defined( CONFIG_LCD_UNICODE ) */
    #ifndef CONFIG_LCD_NO_TERMINAL
      case LCD_ESC:
      {
         if( c == '[' )
         {
            for( i = 0; i < ARRAY_SIZE( parameter ); i++ )
               parameter[i] = 0;
            i = 0;
            TRANSITION( LCD_CTRL );
            return addr;
         }
         break;
      }
      case LCD_CTRL:
      {
         DEBUG_MESSAGE( ": Esc-value = %02X: %c\n", c, c );
         switch( c )
         {
            case 'm':
            {  /* Ignoring video and color sequences. */
               TRANSITION( LCD_NORMAL );
               return addr;
            }
            case 'H':
            {
               TRANSITION( LCD_NORMAL );
               if( i == 0 )
                  return lcdClrscr();
               return lcdGotoXY( parameter[1], parameter[0] );
            }
            case 'J': return addr;
            case 'M':
            {
               TRANSITION( LCD_NORMAL );
               return lcdDelLine( parameter[0] );
            }
            case ';':
            {
               i++;
               if( i >= ARRAY_SIZE( parameter ) )
               {
                  i = 0;
                  ERROR_MESSAGE( ": Not more then %d parameter for escape sequences allowed!\n",
                                  ARRAY_SIZE( parameter ) );
                  TRANSITION( LCD_NORMAL );
               }
               return addr; /* Only 2 parameters allowed. */
            }
            case '?':
            {
               TRANSITION( LCD_CURSOR );
               return addr;
            }
         }
         if( c >= '0' && c <= '9' )
         {
            ASSERT( i < ARRAY_SIZE( parameter ) );
            parameter[i] *= 10;
            parameter[i] += c - '0';
            return addr;
         }
         break;
      }
      case LCD_CURSOR:
      {
         switch( c )
         {
            case '2': /* direct to next case */
            case '5': return addr;
            case 'l':
            {
               TRANSITION( LCD_NORMAL );
               return lcdCursorOff();
            }
            case 'h':
            {
               TRANSITION( LCD_NORMAL );
               return lcdCursorOn();
            }
            //! @todo Esc-sequence for blink-corsor.  
         }
         break;
      }
     #endif /* ifndef CONFIG_LCD_NO_TERMINAL */
      default: break;
   } /* End switch( mg_putchState ) */
   TRANSITION( LCD_NORMAL );
 #endif /* if !(defined( CONFIG_LCD_NO_UMLAUT ) && defined( CONFIG_LCD_NO_TERMINAL )) */

   isLastLine = (MAX_Y()-1 == lcdAddr2Line( addr ));
   DEBUG_MESSAGE( ":addr = %02X c = %c, %02X\n", addr, c, c );
   switch( c )
   {
    #ifndef CONFIG_LCD_NO_UMLAUT
     #ifdef CONFIG_LCD_UNICODE
      case LCD_SPECIAL_CHAR_INTRUDUCHER: TRANSITION( LCD_SPECIAL_CHAR ); return addr;
      case LCD_UMLAUT_INTRUDUCHER:       TRANSITION( LCD_UMLAUT ); return addr;
     #else
      case 0x84: c = LCD_UMLAUT_POSITION_ae; break;
      case 0x8E: c = LCD_UMLAUT_POSITION_Ae; break;
      case 0x99: c = LCD_UMLAUT_POSITION_Oe; break;
      case 0x94: c = LCD_UMLAUT_POSITION_oe; break;
      case 0x81: c = LCD_UMLAUT_POSITION_ue; break;
      case 0x9A: c = LCD_UMLAUT_POSITION_Ue; break;
      case 0x7E: c = LCD_UMLAUT_POSITION_ss; break;
     #endif
    #endif
    #ifndef CONFIG_LCD_NO_TERMINAL
      case ESC:  TRANSITION( LCD_ESC );    return addr;
    #endif
      case '\n':
      case '\r':
      {
         addr = lcdAddr2LineAddr( addr );
       #if (LCD_MAX_Y > 1) || defined(__KERNEL__)
         if( (c == '\n' )
          #ifdef __KERNEL__
            && ( MAX_Y() > 1 )
          #endif
           )
         {
            if( mg_lcdAutoScroll && isLastLine )
               return lcdScrollUp();
           #if (LCD_MAX_Y == 2) && !defined(__KERNEL__)
             addr += LCD_MEM_LINE_SIZE;
           #else
             addr = lcdLine2Addr( lcdAddr2Line( addr ) + 1 );
           #endif

         }
       #endif /* if (LCD_MAX_Y > 1) || defined(__KERNEL__) */
         return lcdSetAddress( addr );
      }
    #ifndef CONFIG_LCD_NO_BACKSPACE
      case '\b': // Backspace
      {
         if( addr == 0 )
            return addr;
       #if (LCD_MAX_Y == 1) && !defined(__KERNEL__)
         addr--;
       #elif (LCD_MAX_Y == 2) && !defined(__KERNEL__)
         if( addr % LCD_MEM_LINE_SIZE == 0 )
            addr -= (LCD_MEM_LINE_SIZE-MAX_X()+1);
         else
            addr--;
       #else
         if( lcdAddr2LineAddr( addr ) == addr )
            addr = lcdLine2Addr( lcdAddr2Line( addr ) - 1 ) + MAX_X()-1;
         else
            addr--;
       #endif
       //#ifdef _LCD_DEL_CHAR_IF_BACKSPACE
         lcdSetAddress( addr );
         lcdSetChar( ' ' );
      // #endif
         return lcdSetAddress( addr );
      }
    #endif
    #if !defined( CONFIG_LCD_NO_VERTICAL_FEED ) && ((LCD_MAX_Y > 1) || defined(__KERNEL__))
      case '\v':
      {
         if( !isLastLine )
         {
          #if (LCD_MAX_Y == 2) && !defined(__KERNEL__)
            addr += LCD_MEM_LINE_SIZE;
          #elif (LCD_MAX_Y == 4) || defined(__KERNEL__)
            addr = lcdLine2Addr( lcdAddr2Line( addr ) + 1 ) + (addr - lcdAddr2LineAddr( addr ));
          #endif
         }
         else if( mg_lcdAutoScroll )
            lcdScrollUp();
         return lcdSetAddress( addr );
      }
    #endif
    #ifdef LCD_BELL_FUNCTION
      case '\a':
      {
         LCD_BELL_FUNCTION();
         return addr;
      }
    #endif
      default:
      {
      #ifdef _CONVERT_CHAR8
         lcdConvertChar8( &c );
      #endif
         break;
      }
   }

   if( mg_lastChar )
   {
      mg_lastChar = FALSE;
      if( mg_lcdAutoScroll )
         addr = lcdScrollUp();
   }
   addr = lcdSetChar( c );
#if (LCD_MAX_Y > 1) || defined(__KERNEL__)
 #ifdef __KERNEL__
   if( MAX_Y() > 1 )
   {
 #endif
#if 1
      if( addr == MAX_X() )
      {
         addr = lcdSetAddress( LCD_MEM_LINE_SIZE );
      }
      else if( addr == (LCD_MEM_LINE_SIZE + MAX_X()) )
      {
      #ifdef __KERNEL__
         if( MAX_Y() > 2 )
         {
            addr = lcdSetAddress( MAX_X() );
         }
         else
         {
            mg_lastChar = TRUE;
            //!! addr = lcdSetAddress( LCD_MEM_LINE_SIZE );
         }
      #else
       #if (LCD_MAX_Y > 2)
         addr = lcdSetAddress( LCD_MAX_X );
       #else
         mg_lastChar = TRUE;
         addr = lcdSetAddress( LCD_MEM_LINE_SIZE );
       #endif
      #endif
      }
    #if (LCD_MAX_Y > 2) || defined(__KERNEL__)
      else if( addr == LCD_MEM_LINE_SIZE )
      {
     #ifdef __KERNEL__
         if( MAX_Y() > 3 )
         {
            addr = lcdSetAddress( LCD_MEM_LINE_SIZE + MAX_X() );
         }
         else
         {
            mg_lastChar = TRUE;
            addr = lcdSetAddress( MAX_X() );
         }
     #else
       #if (LCD_MAX_Y > 3)
         addr = lcdSetAddress( LCD_MEM_LINE_SIZE + LCD_MAX_X );
         break;
       #else
         mg_lastChar = TRUE;
         addr = lcdSetAddress( LCD_MAX_X );
       #endif
     #endif
      }
    #if (LCD_MAX_Y > 3) || defined(__KERNEL__)
      else if( addr == 0 )
      {
         mg_lastChar = TRUE;
         addr = lcdSetAddress( LCD_MEM_LINE_SIZE + MAX_X() );
      }
    #endif /* (LCD_MAX_Y > 2) || defined(__KERNEL__) */
    #endif /* (LCD_MAX_Y > 3) || defined(__KERNEL__) */
#else
     if( (addr % LCD_MEM_LINE_SIZE) == MAX_X() )
     {
        if( (addr / LCD_MEM_LINE_SIZE) == (MAX_Y() - 1) )
           mg_lastChar = TRUE;
        else
           addr = lcdSetAddress( addr - MAX_X() + LCD_MEM_LINE_SIZE );
    }
#endif
 #ifdef __KERNEL__
   }
   else
   {
 #endif
    #else /* (LCD_MAX_Y > 1) || defined(__KERNEL__) */
      if( addr == MAX_X() )
         addr = lcdSetAddress( 0 );
    #endif
 #ifdef __KERNEL__
   }
 #endif
   return addr;
#else /* ifndef _LCD_MINIMAL */
   return lcdSetChar( c );
#endif
}

#ifndef __KERNEL__
/*--------------------------------- lcdPuts -----------------------------------
*/
LCD_CHAR_SIZE_T lcdPuts( const char* str )
{
   LCD_ADDR_T addr = 0;
   LCD_CHAR_SIZE_T n = 0;

   ASSERT( str != NULL );

   while( *str != '\0' && addr >= 0 )
   {
      addr = lcdPutch( *str++ );
      n++;
   }
   return n;
}

#if defined( __HARVARD_ARCH__ ) && !defined( _LCD_NO_FLASH_READING )
/*--------------------------------- lcdPuts -----------------------------------
*/
LCD_CHAR_SIZE_T _lcdPutsP( PGM_P strP )
{
   LCD_ADDR_T addr = 0;
   char c;
   LCD_CHAR_SIZE_T n = 0;

   ASSERT( strP != NULL );

   while( addr >= 0 )
   {
      c = LCD_PGM_READ_BYTE( strP++ );
      if( c == '\0' )
         break;
      addr = lcdPutch( c );
      n++;
   }
   return n;
}
#endif /* defined( __HARVARD_ARCH__ ) && !defined( _LCD_NO_FLASH_READING ) */
#endif /* ifndef __KERNEL__ */

#ifndef _LCD_MINIMAL 
#if defined( CONFIG_ST7066U ) && !defined( CONFIG_LCD_NO_UMLAUT )
/*-----------------------------------------------------------------------------
*/
LCD_ADDR_T lcdLoadUmlaut( void )
{
   LCD_INDEX_T i;
   LCD_ADDR_T addr;
   typedef struct
   {
      const LCD_CGRAM_CHAR_T* pData;
      const BYTE              position;
   } UMLAUT_MAP_T;

   UMLAUT_MAP_T map[3] =
   {
      { &mg_char_Ae, LCD_UMLAUT_POSITION_Ae },
      { &mg_char_Oe, LCD_UMLAUT_POSITION_Oe },
      { &mg_char_Ue, LCD_UMLAUT_POSITION_Ue }
   };

   for( i = 0; i < ARRAY_SIZE( map ); i++ )
   {
      addr = lcdLoadExtraCharP( *map[i].pData, map[i].position );
      if( addr < 0 )
         return addr;
   }

   return addr;
}
#endif /* if defined( CONFIG_ST7066U ) && !defined( CONFIG_LCD_NO_UMLAUT ) */

/*-----------------------------------------------------------------------------
*/
void lcdLoadPredefinedExtraCharacters( void )
{
 #if defined( CONFIG_ST7066U ) && !defined( CONFIG_LCD_NO_UMLAUT )
   lcdLoadUmlaut();
 #endif
 #ifdef CONFIG_LCD_USE_BACKSLASH
   lcdLoadExtraCharP( mg_char_backslash, LCD_BACKSLASH_POSITION );
 #endif
 #ifdef CONFIG_LCD_USE_PARAGRAPH
   lcdLoadExtraCharP( mg_char_paragraph, LCD_PARAGRAPH_POSITION );
 #endif
}
#else
  #define lcdLoadPredefinedExtraCharacters()
#endif /* else ifndef _LCD_MINIMAL */ 
/*--------------------------------- lcdInit -----------------------------------
*/
LCD_ADDR_T lcdInit( void )
{
   LCD_ADDR_T addr;
   LCD_POLL_T poll;

#ifndef __KERNEL__
   LCD_DATA_PORT    &= ~LCD_DATA_MASK;
   LCD_CONTROL_PORT &= ~(LCD_RS_MASK | LCD_RW_MASK | LCD_EN_MASK);
#endif /* ifndef __KERNEL__ */

   LCD_SET_HIGH_IMPEDANCE();
   LCD_SELECT_DATA();
   LCD_MODE_READ();
   LCD_SET_EN_HIGH();

#ifndef _LCD_MINIMAL
 #if !(defined( CONFIG_LCD_NO_UMLAUT ) && defined( CONFIG_LCD_NO_TERMINAL ))
   TRANSITION( LCD_NORMAL );
 #endif
   mg_lcdAutoScroll = TRUE;
#endif /* ifndef _LCD_MINIMAL */
   mg_lastChar = FALSE;
   mg_displayState = 0;
   LCD_INIT_WAIT();
#ifndef _LCD_DATAPORT_PINS_8
   LCD_SELECT_INSTRUCTION();
   _lcdSetData( 0x03 ); // Display = 8 Bit
   LCD_INIT_WAIT();
   lcdSetAddress( 0 );
   _lcdSetData( 0x02 );  // Display = 4 Bit
   _lcdSetData( 0x02 );  // Display = 4 Bit
                          // 2 lines  1 lines
   _lcdSetData( (MAX_Y() > 1)? 0x08 : 0x00 );
 
#else /* ifndef _LCD_DATAPORT_PINS_8 */
   LCD_SELECT_INSTRUCTION();
                           // 2 lines 1 liune
   _lcdSetData( (MAX_Y() > 1)? 0x38 : 0x30 );
#endif /* else of ifndef _LCD_DATAPORT_PINS_8 */
   poll = CONFIG_TLCD_MAX_POLL;
   while( lcdIsBusy( &addr ) )
   {
      //DEBUG_MESSAGE( "%d\n", poll );
      poll--;
      if( poll == 0 )
      {
         ERROR_MESSAGE( ": Unable to initialize LCD\n" );
         return -1;
      }
      LCD_POLL_FUNCTION();
   }
   if( addr < 0 )
      return addr;

   lcdOff();  // Display = off
   lcdSetData( 0x06 );
   lcdLoadPredefinedExtraCharacters();
   lcdClrscr();
   addr = lcdOn();  // Display = on
   return addr;
}

/*================================== EOF ====================================*/
