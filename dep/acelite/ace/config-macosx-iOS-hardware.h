// $Id: config-macosx-iOS-hardware.h 97580 2014-02-11 17:56:01Z sowayaa $
#ifndef ACE_CONFIG_MACOSX_IPHONE_HARDWARE_H
#define ACE_CONFIG_MACOSX_IPHONE_HARDWARE_H

#define ACE_HAS_IPHONE
#define ACE_SIZEOF_LONG_DOUBLE 8

#include "ace/config-macosx-mavericks.h"

#ifdef ACE_HAS_SYSV_IPC
#undef ACE_HAS_SYSV_IPC
#endif

#endif /* ACE_CONFIG_MACOSX_IPHONE_HARDWARE_H */

