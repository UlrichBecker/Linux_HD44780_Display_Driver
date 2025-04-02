/*****************************************************************************/
/*                                                                           */
/*!   @brief Linux driver for HD447800 compatiple alphanumeric displays      */
/*                                                                           */
/*!      To connecting directly by the LCD-device via GPIO-pins              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file    anLcd_drv.c                                                     */
/*! @author  Ulrich Becker                                                   */
/*! @date    17.02.2017                                                      */
/*****************************************************************************/
#include "anLcd_drv.h"
#include "anLcd_dev_tree_names.h"
#include "hd44780Display.h"
#include <linux/slab.h>

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "Ulrich Becker");
MODULE_DESCRIPTION( "HD44780 compatible alphanumeric display-driver" );
MODULE_VERSION( __VERSION );

#if defined( CONFIG_PROC_FS ) || defined(__DOXYGEN__)
   /*! @brief Definition of the name in the process file system. */
   #define PROC_FS_NAME "driver/"DEVICE_BASE_FILE_NAME
#endif

#define PIN_PREFIX_NAME DEVICE_BASE_FILE_NAME

#if !defined( CONFIG_OF ) || defined( CONFIG_AN_LCD_NO_DEV_TREE )
  #define __NO_DEV_TREE
#endif

GLOBAL_T global =
{
   .oLcd =
   {
    #ifdef __NO_DEV_TREE
      .maxX = CONFIG_AN_LCD_MAX_X,
      .maxY = CONFIG_AN_LCD_MAX_Y,
    #endif
      .isInitialized = false,
      .port =
      {
         .rs =
         {
            .name = TS(DT_TAG_RS),
         #ifdef __NO_DEV_TREE
            .number = CONFIG_AN_LCD_GPIO_NUMBER_RS,
         #endif
            .init = INPUT,
            .initialized = FALSE
         },
         .rw =
         {
            .name = TS(DT_TAG_RW),
         #ifdef __NO_DEV_TREE
            .number = CONFIG_AN_LCD_GPIO_NUMBER_RW,
         #endif
            .init = INPUT,
            .initialized = FALSE
         },
         .en =
         {
            .name = TS(DT_TAG_EN),
         #ifdef __NO_DEV_TREE
            .number = CONFIG_AN_LCD_GPIO_NUMBER_EN,
         #endif
            .init = INPUT,
            .initialized = FALSE
         },
         .data =
         {
            {
               .pin.name = TS(DT_TAG_D4),
            #ifdef __NO_DEV_TREE
               .pin.number = CONFIG_AN_LCD_GPIO_NUMBER_D4,
            #endif
               .pin.init = INPUT,
               .pin.initialized = FALSE,
               .mask = (1 << 0)
            },
            {
               .pin.name = TS(DT_TAG_D5),
            #ifdef __NO_DEV_TREE
               .pin.number = CONFIG_AN_LCD_GPIO_NUMBER_D5,
            #endif
               .pin.initialized = FALSE,
               .pin.init = INPUT,
               .mask = (1 << 1)
            },
            {
               .pin.name = TS(DT_TAG_D6),
            #ifdef __NO_DEV_TREE
               .pin.number = CONFIG_AN_LCD_GPIO_NUMBER_D6,
            #endif
               .pin.initialized = FALSE,
               .pin.init = INPUT,
               .mask = (1 << 2)
            },
            {
               .pin.name = TS(DT_TAG_D7),
            #ifdef __NO_DEV_TREE
               .pin.number = CONFIG_AN_LCD_GPIO_NUMBER_D7,
            #endif
               .pin.initialized = FALSE,
               .pin.init = INPUT,
               .mask = (1 << 3)
            }
         } /* .data = */
      } /* .port = */
   }, /* .oLcd = */
   .oWaitQueue.bussy = false
};

/*!----------------------------------------------------------------------------
 */
int writeLcdPort( u8 data )
{
   int i;

   for( i = 0; i < ARRAY_SIZE( global.oLcd.port.data ); i++ )
   {
      if( (global.oLcd.port.data[i].mask & data) != 0 )
         gpio_direction_input( global.oLcd.port.data[i].pin.number );
      else
         gpio_direction_output( global.oLcd.port.data[i].pin.number, 0 );
   }
   return 0;
}

/*!----------------------------------------------------------------------------
 */
BYTE readLcdPort( void )
{
   int i;
   BYTE ret = 0;
   for( i = 0; i < ARRAY_SIZE( global.oLcd.port.data ); i++ )
   {
      if( gpio_get_value( global.oLcd.port.data[i].pin.number ) != 0 )
         ret |= global.oLcd.port.data[i].mask;
   }
   return ret;
}

/* Device file operations begin **********************************************/
/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function open() from the
 *        user-space.
 */
static int onOpen( struct inode* pInode, struct file* pInstance )
{
   DEBUG_MESSAGE( ": Minor-number: %d\n", MINOR(pInode->i_rdev) );
   BUG_ON( pInstance->private_data != NULL );
   atomic_inc( &global.oLcd.openCount );
   DEBUG_MESSAGE( ":   Open-counter: %d\n", 
                  atomic_read( &global.oLcd.openCount ));

   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function close() from the
 *        user-space.
 */
static int onClose( struct inode *pInode, struct file* pInstance )
{
   DEBUG_MESSAGE( ": Minor-number: %d\n", MINOR(pInode->i_rdev) );
   atomic_dec( &global.oLcd.openCount );
   DEBUG_MESSAGE( "   Open-counter: %d\n", 
                  atomic_read( &global.oLcd.openCount ));
   return 0;
}

#ifdef CONFIG_AN_LCD_READBACK
/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function read() from the
 *        user-space.
 * @note The kernel invokes onRead as many times till it returns 0 !!!
 */
static ssize_t onRead( struct file* pInstance,   /*!< @see include/linux/fs.h   */
                       char __user* pBuffer,     /*!< buffer to fill with data */
                       size_t len,               /*!< length of the buffer     */
                       loff_t* pOffset )
{
   char tmp[(MAX_X() * MAX_Y()) * 2 + MAX_Y() + 1];
   char c;
   ssize_t n, i;
   int x, y;
   LCD_ADDR_T oldAddr;

   DEBUG_MESSAGE( ": len = %ld, offset = %lld\n", (long int)len, *pOffset );
   DEBUG_ACCESSMODE( pInstance );

   DEBUG_MESSAGE( "   Open-counter: %d\n", 
                  atomic_read( &global.oLcd.openCount ));

   if( global.oWaitQueue.bussy && ((pInstance->f_flags & O_NONBLOCK) != 0) )
      return -EAGAIN;
   if( wait_event_interruptible( global.oWaitQueue.queue, !global.oWaitQueue.bussy ) )
      return -ERESTARTSYS;


   n = min( len, (sizeof(tmp)-1) );
   n -= (*pOffset);
   if( n <= 0 )
      return n;

   global.oWaitQueue.bussy = true;
   DEBUG_MESSAGE( " n = %d\n", n );

   if( !global.oLcd.isInitialized )
   {
      lcdInit();
      global.oLcd.isInitialized = true;
   }

   if( lcdIsBusy( &oldAddr ) )
   {
      ERROR_MESSAGE( ": LCD seems to be busy...\n" );
      goto L_LIMIT;
   }

   i = 0;
   for( y = 0; y < MAX_Y(); y++ )
   {
      if( lcdSetAddress( lcdLine2Addr( (BYTE)y ) ) < 0 )
         goto L_LIMIT;
      for( x = 0; x < MAX_X(); x++ )
      {
         c = lcdGetChar();
      #ifdef _CONVERT_CHAR8
         lcdConvertBackChar8( &c );
      #endif
      #ifdef CONFIG_LCD_UNICODE
         if( lcdConvertBackSpecialChar( &c ) )
         {
            if( i < (n-1) )
               tmp[i++] = LCD_SPECIAL_CHAR_INTRUDUCHER;
            else
               goto L_LIMIT;
         }
       #ifndef CONFIG_LCD_NO_UMLAUT
         else if( lcdConvertBackUmlaut( &c ) )
         {
            if( i < (n-1) )
               tmp[i++] = LCD_UMLAUT_INTRUDUCHER;
            else
               goto L_LIMIT;
         }
       #endif /* ifndef CONFIG_LCD_NO_UMLAUT */
      #endif /* ifdef CONFIG_LCD_UNICODE */
         if( i < n )
           tmp[i++] = c;
         else
           goto L_LIMIT;
      }
      if( i < n )
         tmp[i++] = '\n';
      else
         break;
   }

L_LIMIT:
   tmp[i] = '\0';
   global.oWaitQueue.bussy = false;
   lcdSetAddress( oldAddr );

   DEBUG_MESSAGE( " i = %d\n", i );
   if( copy_to_user( pBuffer, tmp, i ) != 0 )
   {
      ERROR_MESSAGE( "copy_to_user\n" );
      global.oWaitQueue.bussy = false;
      return -EFAULT;
   }

   (*pOffset) += n;
   global.oWaitQueue.bussy = false;
   return i;
   /* Number of bytes successfully read. */
}
#endif /* ifdef CONFIG_AN_LCD_READBACK */

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function write() from the
 *        user-space.
 */
static ssize_t onWrite( struct file *pInstance,
                        const char __user* pBuffer,
                        size_t len,
                        loff_t* pOffset )
{
   size_t notCopied;
   DEBUG_MESSAGE( ": len = %ld, offset = %lld\n", (long int)len, *pOffset );
   DEBUG_ACCESSMODE( pInstance );
   DEBUG_MESSAGE( "   Open-counter: %d\n", 
                  atomic_read( &global.oLcd.openCount ));

   if( global.oWaitQueue.bussy && ((pInstance->f_flags & O_NONBLOCK) != 0) )
      return -EAGAIN;
   if( wait_event_interruptible( global.oWaitQueue.queue, !global.oWaitQueue.bussy ) )
      return -ERESTARTSYS;

   global.oWaitQueue.bussy = true;

   global.oBuffer.len = min( len, global.oBuffer.capacity );
   notCopied = copy_from_user( global.oBuffer.pData, pBuffer, global.oBuffer.len );
   global.oBuffer.len -= notCopied;

   if( global.oBuffer.len == 0 )
   {
      global.oWaitQueue.bussy = false;
      return 0;
   }

   queue_work( global.oWorkQueue.poWorkqueue, &global.oWorkQueue.oWrite );
   return global.oBuffer.len;
}

/*=========================== ioctl handling ================================*/
/*!----------------------------------------------------------------------------
 */
static long onIoctlReset( unsigned long arg )
{
   if( lcdInit() < 0 )
      return -EFAULT;

   global.oLcd.isInitialized = true;
   return 0;
}

/*!----------------------------------------------------------------------------
 */
static long onIoctlScrollUp( unsigned long arg )
{
   if( lcdScrollUp() < 0 )
      return -EFAULT;
   return 0;
}

/*!----------------------------------------------------------------------------
 */
static long onIoctlScrollDown( unsigned long arg )
{
   if( lcdScrollDown() < 0 )
      return -EFAULT;
   return 0;
}

/*!----------------------------------------------------------------------------
 */
static long onIoctlLoadDefaultCgRam( unsigned long arg )
{
   LCD_ADDR_T addr;

   if( lcdIsBusy( &addr ) )
   {
      ERROR_MESSAGE( ": LCD is still busy!\n" );
      return -EFAULT;
   }
   lcdLoadPredefinedExtraCharacters();
   if( lcdSetAddress( addr ) < 0 )
      return -EFAULT;

   return 0;
}

/*!----------------------------------------------------------------------------
 */
static long onIoctlWriteCgRam( unsigned long arg )
{
   LCD_CGRAM_T cgRamBuffer;
   int ret = 0;
   LCD_ADDR_T addr;

   if( lcdIsBusy( &addr ) )
   {
      ERROR_MESSAGE( ": LCD is still busy!\n" );
      return -EFAULT;
   }

   if( copy_from_user( &cgRamBuffer, (void*)arg, sizeof( cgRamBuffer ) ) != 0 )
   {
      ERROR_MESSAGE( ": copy_from_user failed\n" );
      return -EFAULT;
   }

   if( lcdLoadExtraCharP( cgRamBuffer.content, cgRamBuffer.address ) < 0 )
   {
      ERROR_MESSAGE( ": writing of CG-RAM failed\n" );
      ret = -EFAULT;
   }

   if( lcdSetAddress( addr ) < 0 )
      return -EFAULT;

   return ret;
}

/*!----------------------------------------------------------------------------
 */
static long onIoctlAutoScrollOn( unsigned long arg )
{
   lcdEnableAutoScroll();
   return 0;
}

/*!----------------------------------------------------------------------------
 */
static long onIoctlAutoScrollOff( unsigned long arg )
{
   lcdDisableAutoScroll();
   return 0;
}

/*!----------------------------------------------------------------------------
 */
static long onIoctlDisplayOff( unsigned long arg )
{
   if( lcdOff() < 0 )
      return -EFAULT;
   return 0;
}

/*!----------------------------------------------------------------------------
 */
static long onIoctlDisplayOn( unsigned long arg )
{
   if( lcdOn() < 0 )
      return -EFAULT;
   return 0;
}

/*!----------------------------------------------------------------------------
 */
typedef struct
{
   char*        name;
   unsigned int number;
   long (*function)( unsigned long arg );
} IOC_INFO_T;

#define IOCTL_ITEM( n, f ) { #n, n, f }

/*!
 * @todo Ioctl-dummy handling of TCGETS and TCSETS defined in "termios.h"
 */

const static IOC_INFO_T mg_ioctlList[] =
{
   IOCTL_ITEM( AN_DISPLAY_IOC_RESET,              onIoctlReset ),
   IOCTL_ITEM( AN_DISPLAY_IOC_SCROLL_UP,          onIoctlScrollUp ),
   IOCTL_ITEM( AN_DISPLAY_IOC_SCROLL_DOWN,        onIoctlScrollDown ),
   IOCTL_ITEM( AN_DISPLAY_IOC_LOAD_DEFAULT_CGRAM, onIoctlLoadDefaultCgRam ),
   IOCTL_ITEM( AN_DISPLAY_IOC_WRITE_CGRAM,        onIoctlWriteCgRam ),
   IOCTL_ITEM( AN_DISPLAY_IOC_AUTOSCROLL_ON,      onIoctlAutoScrollOn ),
   IOCTL_ITEM( AN_DISPLAY_IOC_AUTOSCROLL_OFF,     onIoctlAutoScrollOff ),
   IOCTL_ITEM( AN_DISPLAY_IOC_OFF,                onIoctlDisplayOff ),
   IOCTL_ITEM( AN_DISPLAY_IOC_ON,                 onIoctlDisplayOn ),
   { NULL, 0, NULL }
};

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function ioctrl() from the
 *        user-space.
 */
static long onIoctrl( struct file* pInstance,
                      unsigned int cmd,
                      unsigned long arg )
{
   int ret;
   const IOC_INFO_T* pCurrentItem;

   DEBUG_MESSAGE( ": cmd = 0x%08X arg = 0x%08lX\n", cmd, arg );
   DEBUG_ACCESSMODE( pInstance );
   DEBUG_MESSAGE( "   Open-counter: %d\n",
                   atomic_read( &global.oLcd.openCount ));

   if( global.oWaitQueue.bussy && ((pInstance->f_flags & O_NONBLOCK) != 0) )
      return -EAGAIN;
   if( wait_event_interruptible( global.oWaitQueue.queue, !global.oWaitQueue.bussy ) )
      return -ERESTARTSYS;
   global.oWaitQueue.bussy = true;

   for( pCurrentItem = mg_ioctlList; pCurrentItem->function != NULL; pCurrentItem++ )
   {
      if( pCurrentItem->number != cmd )
         continue;
      DEBUG_MESSAGE( ": execute ioctl-command: %s\n", pCurrentItem->name );
      ret = pCurrentItem->function( arg );
      if( ret < 0 )
         ERROR_MESSAGE( ": executing of ioctl-command %s failed!\n",
                        pCurrentItem->name );
      break;
   }
   if( pCurrentItem->function == NULL )
   {
      ERROR_MESSAGE( ": Unknown ioctl-command: 0x%08X\n", cmd );
      ret = -EINVAL;
   }

   global.oWaitQueue.bussy = false;
   return ret;
}

/*===========================================================================*/
/*!----------------------------------------------------------------------------
 */
static struct file_operations global_fops =
{
  .owner          = THIS_MODULE,
  .open           = onOpen,
  .release        = onClose,
#ifdef CONFIG_AN_LCD_READBACK
  .read           = onRead,
#endif
  .write          = onWrite,
  .unlocked_ioctl = onIoctrl
};
/* Device file operations end ************************************************/

/*!----------------------------------------------------------------------------
 */
static void onWorkqueueInit( struct work_struct* poWork )
{
   DEBUG_MESSAGE( "\n" );
   lcdInit();
   global.oLcd.isInitialized = true;
   global.oWaitQueue.bussy = false;
   wake_up_interruptible( &global.oWaitQueue.queue );
}

/*!----------------------------------------------------------------------------
 */
static void onWorkqueueWrite( struct work_struct* poWork )
{
   size_t i;
   u8* pData = global.oBuffer.pData;
   DEBUG_MESSAGE( "\n" );

   if( !global.oLcd.isInitialized )
   {
      lcdInit();
      global.oLcd.isInitialized = true;
   }

   for( i = 0; i < global.oBuffer.len; i++ )
   {
      if( *pData == '\0' )
         break;
      lcdPutch( *pData );
      pData++;
   }

   global.oWaitQueue.bussy = false;
   wake_up_interruptible( &global.oWaitQueue.queue );
}


/* Process-file-system begin *************************************************/
#ifdef CONFIG_PROC_FS
/*-----------------------------------------------------------------------------
 */
static int procOnOpen( struct seq_file* pSeqFile, void* pValue )
{
   int i;
   const IOC_INFO_T* pCurrentItem;

   DEBUG_MESSAGE( "\n" );
   seq_printf( pSeqFile, KBUILD_MODNAME " %dx%d Version: " __VERSION "\n\n",
               global.oLcd.maxX, global.oLcd.maxY );

   for( i = 0; i < ARRAY_SIZE( global.oLcd.port.list ); i++ )
   {
      seq_printf( pSeqFile, "GPIO %02d: %s = %s\n",
                  global.oLcd.port.list[i]->number,
                  global.oLcd.port.list[i]->name,
                 (gpio_get_value( global.oLcd.port.list[i]->number ) != 0)?
                 "high" : "low" );
   }

   seq_printf( pSeqFile, "\nValid commands for ioctl():\n" ); 

   for( pCurrentItem = mg_ioctlList; pCurrentItem->function != NULL; pCurrentItem++ )
   {
      seq_printf( pSeqFile, "%s:\t0x%08X\n",
                  pCurrentItem->name,
                  pCurrentItem->number );
   }

   seq_printf( pSeqFile, "\nAuto scroll: %s\n",
               lcdIsAutoScroll()? "enabled" : "disabled" );
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static int _procOnOpen( struct inode* pInode, struct file *pFile )
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
   return single_open( pFile, procOnOpen, NULL );
#else
   return single_open( pFile, procOnOpen, PDE_DATA( pInode ) );
#endif
}

/*-----------------------------------------------------------------------------
 */
static ssize_t procOnWrite( struct file* seq, const char* pData,
                            size_t len, loff_t* pPos )
{
#ifdef CONFIG_DEBUG_AN_LCD
   BYTE addrOut;
#endif

   DEBUG_MESSAGE( "\n" );
   if( wait_event_interruptible( global.oWaitQueue.queue, !global.oWaitQueue.bussy ) )
      return -ERESTARTSYS;

   global.oWaitQueue.bussy = true;
   if( *pData == '1' )
      lcdInit();

#ifdef CONFIG_DEBUG_AN_LCD
   /* IO-Test */
   if( *pData == '2' ) 
   {
      for( addrOut = 0; addrOut <= 0x0F; addrOut++ )
         lcdSetAddress( addrOut );
   }
#endif
   global.oWaitQueue.bussy = false;
   wake_up_interruptible( &global.oWaitQueue.queue );
   return len;
}

/*-----------------------------------------------------------------------------
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static const struct proc_ops global_procFileOps =
{
  .proc_open    = _procOnOpen,
  .proc_read    = seq_read,
  .proc_write   = procOnWrite,
  .proc_lseek   = seq_lseek,
  .proc_release = single_release
};
#else
static const struct file_operations global_procFileOps =
{
  .owner   = THIS_MODULE,
  .open    = _procOnOpen,
  .read    = seq_read,
  .write   = procOnWrite,
  .llseek  = seq_lseek,
  .release = single_release
};
#endif

#endif /* ifdef CONFIG_PROC_FS */
/* Process-file-system end ***************************************************/

/* Power management functions begin ******************************************/
#ifdef CONFIG_PM_
/*-----------------------------------------------------------------------------
 */
static int onPmSuspend( struct device* pDev, pm_message_t state )
{
   DEBUG_MESSAGE( "( %p )\n", pDev );
#ifdef CONFIG_DEBUG_AN_LCD
   #define CASE_ITEM( s ) case s: DEBUG_MESSAGE( ": " #s "\n" ); break;
   switch( state.event )
   {
      CASE_ITEM( PM_EVENT_ON )
      CASE_ITEM( PM_EVENT_FREEZE )
      CASE_ITEM( PM_EVENT_SUSPEND )
      CASE_ITEM( PM_EVENT_HIBERNATE )
      default:
      {
         DEBUG_MESSAGE( "pm_event: 0x%X\n", state.event );
         break;
      }
  }
  #undef CASE_ITEM
#endif
  return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onPmResume( struct device* pDev )
{
  DEBUG_MESSAGE( "(%p)\n", pDev );
  return 0;
}

#endif /* ifdef CONFIG_PM_ */
/* Power management functions end ********************************************/


#ifndef __NO_DEV_TREE
/*!----------------------------------------------------------------------------
 * 
 */
static int __init readDviceTreeNode( void )
{
   typedef struct
   {
      int*        pNumber;
      const char* name;
   } LIST;

   LIST list[ARRAY_SIZE( global.oLcd.port.list ) + 2];
   struct device_node* pNode;
   const void* pProperty;
   int i;
   int len;
   int ret = 0;

   pNode = of_find_node_by_name( NULL, DEVICE_BASE_FILE_NAME );
   if( pNode == NULL )
   {
      ERROR_MESSAGE( ": Devicenode " DEVICE_BASE_FILE_NAME " not found!\n" );
      return -1;
   }

   for( i = 0; i < ARRAY_SIZE( global.oLcd.port.list ); i++ )
   {
      list[i].pNumber = &global.oLcd.port.list[i]->number;
      list[i].name    = global.oLcd.port.list[i]->name;
   }
   list[i].pNumber = &global.oLcd.maxX;
   list[i].name    = TS(DT_TAG_X);
   i++;
   list[i].pNumber = &global.oLcd.maxY;
   list[i].name    = TS(DT_TAG_Y);

   for( i = 0; i < ARRAY_SIZE( list ); i++ )
   {
      pProperty = of_get_property( pNode, list[i].name, &len );
      if( pProperty == NULL )
      {
         ERROR_MESSAGE( ": Could not found property \"%s\" of "
                        DEVICE_BASE_FILE_NAME,
                        list[i].name );
         ret = -1;
      }
      else
      {
         *(list[i].pNumber) = (int)of_read_ulong( pProperty, 1 );
         DEBUG_MESSAGE( ": Property: \"%s\", value: %d\n",
                         list[i].name,
                         *list[i].pNumber );
      }
   }

   return ret;
}
#endif /* ifndef __NO_DEV_TREE */

/*!----------------------------------------------------------------------------
 */
static void releasePort( void )
{
   int i;
   for( i = 0; i < ARRAY_SIZE( global.oLcd.port.list ); i++ )
   {
      if( global.oLcd.port.list[i]->initialized )
      {
         global.oLcd.port.list[i]->initialized = FALSE;
         gpio_free( global.oLcd.port.list[i]->number );
         DEBUG_MESSAGE( ": free GPIO-number %02d name: %s\n",
                        global.oLcd.port.list[i]->number,
                        global.oLcd.port.list[i]->name );
      }
   }
}

/*!----------------------------------------------------------------------------
 * 
 */
static int __init requestPort( void )
{
   int i, ret;
   for( i = 0; i < ARRAY_SIZE( global.oLcd.port.list ); i++ )
   {
      if( global.oLcd.port.list[i]->initialized )
         continue;
      ret = gpio_request( global.oLcd.port.list[i]->number,
                          global.oLcd.port.list[i]->name );
      if( ret != 0 )
      {
         ERROR_MESSAGE( ": %d: Unable to request GPIO-number %02d name: %s\n",
                        ret,
                        global.oLcd.port.list[i]->number,
                        global.oLcd.port.list[i]->name );
         releasePort();
         return ret;
      }
      global.oLcd.port.list[i]->initialized = TRUE;
      if( global.oLcd.port.list[i]->init == INPUT )
         ret = gpio_direction_input( global.oLcd.port.list[i]->number );
      else
         ret = gpio_direction_output( global.oLcd.port.list[i]->number,
                                      (global.oLcd.port.list[i]->init == OUTPUT_LOW)? 0 : 1 );
      if( ret != 0 )
      {
         ERROR_MESSAGE( ": %d: Unable to set direction of GPIO-number %02d name: %s\n",
                         ret,
                         global.oLcd.port.list[i]->number,
                         global.oLcd.port.list[i]->name );
         releasePort();
         return ret;
      }
      DEBUG_MESSAGE( ": GPIO-number %02d name: %s initialized\n",
                     global.oLcd.port.list[i]->number,
                     global.oLcd.port.list[i]->name );
   }
   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief Driver constructor
 */
static int __init driverInit( void )
{
   DEBUG_MESSAGE( "*** Loading driver \"" DEVICE_BASE_FILE_NAME "\" ***\n" );

   if( alloc_chrdev_region( &global.deviceNumber, 0, 1, DEVICE_BASE_FILE_NAME ) < 0 )
   {
      ERROR_MESSAGE( "alloc_chrdev_region\n" );
      return -EIO;
   }

   global.pObject = cdev_alloc();
   if( global.pObject == NULL )
   {
      ERROR_MESSAGE( "cdev_alloc\n" );
      goto L_DEVICE_NUMBER;
   }

   global.pObject->owner = THIS_MODULE;
   global.pObject->ops = &global_fops;
   if( cdev_add( global.pObject, global.deviceNumber, 1 ) )
   {
      ERROR_MESSAGE( "cdev_add\n" );
      goto L_REMOVE_DEV;
   }

  /*!
   * Register of the driver-instances visible in /sys/class/DEVICE_BASE_FILE_NAME
   */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
   global.pClass = class_create( DEVICE_BASE_FILE_NAME );
#else
   global.pClass = class_create( THIS_MODULE, DEVICE_BASE_FILE_NAME );
#endif
   if( IS_ERR(global.pClass) )
   {
      ERROR_MESSAGE( "class_create: No udev support\n" );
      goto L_CLASS_REMOVE;
   }

   if( device_create( global.pClass,
                      NULL,
                      global.deviceNumber,
                      NULL,
                      DEVICE_BASE_FILE_NAME )
      == NULL )
   {
      ERROR_MESSAGE( "device_create: " DEVICE_BASE_FILE_NAME "\n" );
      goto L_INSTANCE_REMOVE;
   }
   global.oLcd.minor = 0;
   atomic_set( &global.oLcd.openCount, 0 );
   DEBUG_MESSAGE( ": Instance " DEVICE_BASE_FILE_NAME " created\n" );

#ifdef CONFIG_PM_
  global.pClass->suspend = onPmSuspend;
  global.pClass->resume =  onPmResume;
#endif

#ifdef CONFIG_PROC_FS
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
   global.poProcFile = proc_create_data( PROC_FS_NAME,
                                         S_IRUGO | S_IWUGO,
                                         NULL,
                                         &global_procFileOps,
                                         NULL );
 #else
   global.poProcFile = proc_create( PROC_FS_NAME,
                                    S_IRUGO | S_IWUGO,
                                    NULL,
                                    &global_procFileOps );
 #endif
   if( global.poProcFile == NULL )
   {
      ERROR_MESSAGE( "Unable to create proc entry: /proc/" PROC_FS_NAME " !\n" );
      //goto L_INSTANCE_REMOVE;
      /*
       * For the case there is an old entry in the proc-filesystem it`s better
       * to jump to remove_proc_entry().
       */
      goto L_PROC_ENTRY_REMOVE;
   }
#endif

   global.oWorkQueue.poWorkqueue = create_workqueue( KBUILD_MODNAME );
   if( global.oWorkQueue.poWorkqueue == NULL )
   {
      ERROR_MESSAGE( "Unable to create work-queue!\n" );
    #ifdef CONFIG_PROC_FS
      goto L_PROC_ENTRY_REMOVE;
    #else
      goto L_INSTANCE_REMOVE;
    #endif
   }

   INIT_WORK( &global.oWorkQueue.oInit, onWorkqueueInit );
   INIT_WORK( &global.oWorkQueue.oWrite, onWorkqueueWrite );

   global.oLcd.port.list[0] = &global.oLcd.port.rs;
   global.oLcd.port.list[1] = &global.oLcd.port.rw;
   global.oLcd.port.list[2] = &global.oLcd.port.en;
   global.oLcd.port.list[3] = &global.oLcd.port.data[0].pin;
   global.oLcd.port.list[4] = &global.oLcd.port.data[1].pin;
   global.oLcd.port.list[5] = &global.oLcd.port.data[2].pin;
   global.oLcd.port.list[6] = &global.oLcd.port.data[3].pin;
#ifndef __NO_DEV_TREE
   if( readDviceTreeNode() != 0 )
      goto L_WORKQUEUE_REMOVE;
#endif
   if( requestPort() != 0 )
      goto L_WORKQUEUE_REMOVE;

   init_waitqueue_head( &global.oWaitQueue.queue );

   global.oBuffer.len = 0;
   global.oBuffer.capacity = global.oLcd.maxX * global.oLcd.maxY * 2;
   global.oBuffer.pData = kmalloc( global.oBuffer.capacity, GFP_KERNEL );
   if( global.oBuffer.pData == NULL )
   {
      ERROR_MESSAGE( "Unable to alloc kernel-memory of %d bytes!\n",
                     global.oBuffer.capacity );
      goto L_WORKQUEUE_REMOVE;
   }

   global.oWaitQueue.bussy = true;
   queue_work( global.oWorkQueue.poWorkqueue, &global.oWorkQueue.oInit );

   DEBUG_MESSAGE( "success\n" );
   return 0;

L_WORKQUEUE_REMOVE:
   DEBUG_MESSAGE( "destroy_workqueue()\n" );
   destroy_workqueue( global.oWorkQueue.poWorkqueue );

#ifdef CONFIG_PROC_FS
L_PROC_ENTRY_REMOVE:
   DEBUG_MESSAGE( "remove_proc_entry()\n" );
   remove_proc_entry( PROC_FS_NAME, NULL );
#endif

L_INSTANCE_REMOVE:
   DEBUG_MESSAGE( "device_destroy()\n" );
   device_destroy( global.pClass, global.deviceNumber );

L_CLASS_REMOVE:
   DEBUG_MESSAGE( "class_destroy()\n" );
   class_destroy( global.pClass );

L_REMOVE_DEV:
   DEBUG_MESSAGE( "kobject_put()\n" );
   kobject_put( &global.pObject->kobj );

L_DEVICE_NUMBER:
   DEBUG_MESSAGE( "unregister_chrdev_region()\n" );
   unregister_chrdev_region( global.deviceNumber, 1 );

   DEBUG_MESSAGE( "Failed to load driver \"" DEVICE_BASE_FILE_NAME "\"\n" );
   return -EIO;
}

/*!----------------------------------------------------------------------------
 * @brief Driver destructor
 */
static void __exit driverExit( void )
{
   DEBUG_MESSAGE( "*** Removing driver \"" DEVICE_BASE_FILE_NAME "\" ***\n" );

  //cancel_work( &global.oWorkQueue.oInit );
   destroy_workqueue( global.oWorkQueue.poWorkqueue );

  
   lcdOff();

  
   releasePort();
#ifdef CONFIG_PROC_FS
   remove_proc_entry( PROC_FS_NAME, NULL );
#endif
   device_destroy( global.pClass, global.deviceNumber );
   class_destroy( global.pClass );
   cdev_del( global.pObject );
   unregister_chrdev_region( global.deviceNumber, 1 );
   kfree( global.oBuffer.pData );
}

/*-----------------------------------------------------------------------------
 */
module_init( driverInit );
module_exit( driverExit );
/*================================== EOF ====================================*/
