/*****************************************************************************/
/*                                                                           */
/*!   @brief Linux driver for HD447800 compatiple alphanumeric displays      */
/*                                                                           */
/*!      To connecting directly by the LCD-device via GPIO-pins              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file    anLcd_drv.h                                                     */
/*! @see     anLcd_drv.c                                                     */
/*! @see     hd44780Display.c                                                */
/*! @author  Ulrich Becker                                                   */
/*! @date    17.02.2017                                                      */
/*****************************************************************************/
#ifndef _ANLCD_DRV_H
#define _ANLCD_DRV_H

#include <linux/version.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#ifdef CONFIG_PROC_FS
   #include <linux/proc_fs.h>
   #include <linux/seq_file.h>
#endif

#include "an_disp_ioctl.h"

#define DEVICE_BASE_FILE_NAME KBUILD_MODNAME

#ifdef TO_STRING_LITERAL
   #undef TO_STRING_LITERAL
#endif
#ifdef TO_STRING
   #undef TO_STRING
#endif
#define TO_STRING_LITERAL( s ) # s 
#define TO_STRING( s ) TO_STRING_LITERAL( s )

#ifndef VERSION
   #error Macro VERSION for version-string is not defined!
#endif

#define __VERSION TO_STRING( VERSION )


/* Begin of message helper macros for "dmesg" *********************************/
/* NOTE for newer systems with "systend" 
 * "dmesg -w" corresponds the old “tail -f /var/log/messages”
 */
#define ERROR_MESSAGE( constStr, n... ) \
   printk( KERN_ERR DEVICE_BASE_FILE_NAME "-systemerror %d: %s: " constStr, __LINE__, __func__, ## n )

#if defined( CONFIG_DEBUG_AN_LCD ) || defined(__DOXYGEN__)
   #define DEBUG_MESSAGE( constStr, n... ) \
      printk( KERN_DEBUG DEVICE_BASE_FILE_NAME "-dbg %d: %s: " constStr, __LINE__, __func__, ## n )

   #define DEBUG_ACCESSMODE( pFile ) \
      DEBUG_MESSAGE( ": access: %s\n", \
                     (pFile->f_flags & O_NONBLOCK)? "non blocking":"blocking" )
#else
   #define DEBUG_MESSAGE( constStr, n... )
   #define DEBUG_ACCESSMODE( pFile )
#endif

#define INFO_MESSAGE( constStr, n... ) \
   printk( KERN_INFO DEVICE_BASE_FILE_NAME ": " constStr, ## n )

/* End of message helper macros for "dmesg" ++++++++***************************/

#define ASSERT( condition ) BUG_ON( !(condition) )

typedef u8   BYTE;
typedef u16  WORD;
typedef bool BOOL;
#define FALSE false
#define TRUE  true
#define FLASH const
#define INLINE inline
#define ESC 0x1B

typedef enum
{
   INPUT,
   OUTPUT_LOW,
   OUTPUT_HIGH
} IO_INIT_T;

typedef struct
{
   const char* name;
   int         number;
   IO_INIT_T   init;
   BOOL        initialized;
} LCD_GPIO_PIN_T;

typedef struct
{
   LCD_GPIO_PIN_T pin;
   u8             mask;
} LCD_DATA_IO_T;

typedef struct
{
   LCD_GPIO_PIN_T rs;
   LCD_GPIO_PIN_T rw;
   LCD_GPIO_PIN_T en;
   LCD_DATA_IO_T data[4];
   LCD_GPIO_PIN_T* list[7];
} LCD_IO_PORT_T;

typedef struct
{
   size_t capacity;
   size_t len;
   u8*    pData;
} BUFFER_T;

/*!
 * @brief Object-type of private-data for each driver-instance.
 */
typedef struct
{
   int           minor;
   atomic_t      openCount;
   int           maxX;
   int           maxY;
   bool          isInitialized;
   bool          autoScroll;
   bool          lastChar;
   BYTE          displayState;
   LCD_IO_PORT_T port;
} LCD_OBJ_T;

typedef struct 
{
   struct workqueue_struct* poWorkqueue;
   struct work_struct       oInit;
   struct work_struct       oWrite;
} WORK_QUEUE_T;

typedef struct WAIT_QUEUE_T
{
   volatile bool     bussy;
   wait_queue_head_t queue;
} WAIT_QUEUE_T;

/*!
 * @brief Structure of global variables.
 *
 * Of course it's not necessary to put the following variables in a structure
 * but that is a certain kind of commenting the code its self and makes the
 * code better readable.
 */
typedef struct
{
   dev_t             deviceNumber;
   struct cdev*      pObject;
   struct class*     pClass;
   WAIT_QUEUE_T      oWaitQueue;
   WORK_QUEUE_T      oWorkQueue; 
   LCD_OBJ_T         oLcd;
   BUFFER_T          oBuffer;
#ifdef CONFIG_PROC_FS
   struct proc_dir_entry*  poProcFile;
#endif
} GLOBAL_T;

extern GLOBAL_T global;

int writeLcdPort( u8 data );
BYTE readLcdPort( void );

#endif /* ifndef _ANLCD_DRV_H */
/*================================== EOF ====================================*/
