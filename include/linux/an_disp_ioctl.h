/*****************************************************************************/
/*                                                                           */
/*!  @brief Commands for ioctl() to controlling the alphanumeric HD44780     */
/*!         compatible display-driver. /dev/anLcd                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file     an_disp_ioctl.h                                                */
/*! @author   Ulrich Becker                                                  */
/*! @date     21.03.2017                                                     */
/*  Revision:                                                                */
/*****************************************************************************/
#ifndef _AN_DISP_IOCTL_H
#define _AN_DISP_IOCTL_H
#include <linux/ioctl.h>

#ifndef STATIC_ASSERT
  #define __STATIC_ASSERT__( condition, line ) \
       extern char static_assertion_on_line_##line[2*((condition)!=0)-1];

  #define STATIC_ASSERT( condition ) __STATIC_ASSERT__( condition, __LINE__)
#endif

typedef unsigned char LCD_CGRAM_CHAR_T[8];

typedef struct
{
   unsigned char     address;
   LCD_CGRAM_CHAR_T  content;
} __attribute__ ((packed)) 
LCD_CGRAM_T;

STATIC_ASSERT( sizeof( LCD_CGRAM_T ) == 9 );

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

#define AN_DISPLAY_IOC_MAGIC  'd'

#define AN_DISPLAY_IOC_RESET               _IO( AN_DISPLAY_IOC_MAGIC, 0 )
#define AN_DISPLAY_IOC_SCROLL_UP           _IO( AN_DISPLAY_IOC_MAGIC, 1 )
#define AN_DISPLAY_IOC_SCROLL_DOWN         _IO( AN_DISPLAY_IOC_MAGIC, 2 )
#define AN_DISPLAY_IOC_AUTOSCROLL_ON       _IO( AN_DISPLAY_IOC_MAGIC, 3 )
#define AN_DISPLAY_IOC_AUTOSCROLL_OFF      _IO( AN_DISPLAY_IOC_MAGIC, 4 )
#define AN_DISPLAY_IOC_ON                  _IO( AN_DISPLAY_IOC_MAGIC, 5 )
#define AN_DISPLAY_IOC_OFF                 _IO( AN_DISPLAY_IOC_MAGIC, 6 )
#define AN_DISPLAY_IOC_LOAD_DEFAULT_CGRAM  _IO( AN_DISPLAY_IOC_MAGIC, 7 )
#define AN_DISPLAY_IOC_WRITE_CGRAM         _IOW( AN_DISPLAY_IOC_MAGIC, 8, LCD_CGRAM_T )


#endif /* ifndef _AN_DISP_IOCTL_H */
/*================================== EOF ====================================*/
