/*****************************************************************************/
/*                                                                           */
/*!    @brief Definition of the devicetree-names for kernelmodule tlcd       */
/*                                                                           */
/*!         Common Header included in the related .dts -file and             */
/*!         anLcd_drv.c                                                      */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file     anLcd_dev_tree_names.h                                         */
/*! @see      anLcd_XXX.dts                                                  */
/*! @see      anLcd_drv.c                                                    */
/*! @author   Ulrich Becker                                                  */
/*! @date     04.03.2017                                                     */
/*  Revision:                                                                */
/*****************************************************************************/
#ifndef _TLCD_TREE_NAMES_H
#define _TLCD_TREE_NAMES_H

#define DT_TAG_X   maxX
#define DT_TAG_Y   maxY

#define DT_TAG_RS  rs
#define DT_TAG_RW  rw
#define DT_TAG_EN  en
#define DT_TAG_D4  d4
#define DT_TAG_D5  d5
#define DT_TAG_D6  d6
#define DT_TAG_D7  d7

#define __TS( s ) #s
#define TS( s ) __TS( s )

#endif /* ifndef _TLCD_TREE_NAMES_H */
/*================================== EOF ====================================*/
