###############################################################################
##                                                                           ##
##      Yocto recipe (proposal) to build out of tree kernel-module for       ##
##      HD44780 compatible alphanumeric displays logical connected           ##
##      directly to GPIO                                                     ##
##                                                                           ##
##---------------------------------------------------------------------------##
## File:     anlcd-mod_0.1.bb                                                ##
## Author:   Ulrich Becker                                                   ##
## Date:     02.07.2018                                                      ##
## Revision:                                                                 ##
###############################################################################
#TODO: Handling of device-tree

SUMMARY = "Recipe to build external Linux kernel module for a alphanumeric" \
          "HD44780 compatible display"
AUTHOR  = "Ulrich Becker"
SECTION = "modules"
LICENSE = "GPLv3"

SRCREV           = "${AUTOREV}"
PV               = "1.0-git${SRCPV}"
PR               = "r0"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.
inherit module

SRC_URI = "git://github.com/UlrichBecker/Linux_HD44780_Display_Driver.git"

S = "${WORKDIR}/git"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=84dcc94da3adb52b53ae4fa38fe49e5d"

# This puts the module name into /etc/modules-load.d/modname.conf on the image.
KERNEL_MODULE_AUTOLOAD += "anLcd"

FILES_${PN}      += "${sysconfdir}/udev/rules.d/*.rules"
FILES_${PN}-dev  += "${includedir}/linux/*.h ${docdir}/anLcd/*.md"
FILES_${PN}-sdk  += "${includedir}/linux/*.h ${docdir}/anLcd/*.md"

do_compile() {
   oe_runmake -C ${S}/ksrc all
}

do_install() {
   oe_runmake -C ${S}/ksrc INSTALL_MOD_PATH=${D} modules_install
   mkdir -p ${D}${includedir}/linux
   cp ${S}/include/linux/an_disp_ioctl.h ${D}${includedir}/linux/an_disp_ioctl.h
   mkdir -p ${D}${sysconfdir}/udev/rules.d
   cp ${S}/BR_overlay/etc/udev/rules.d/50-anLcd.rules ${D}${sysconfdir}/udev/rules.d/50-anLcd.rules
   mkdir -p ${D}${docdir}/anLcd
   cp ${S}/README.md ${D}${docdir}/anLcd/README.md
}

#=================================== EOF ======================================
