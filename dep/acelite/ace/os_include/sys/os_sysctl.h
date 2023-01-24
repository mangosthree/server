// -*- C++ -*-

//=============================================================================
/**
 *  @file    os_sysctl.h
 *
 *  declarations for sysctl
 *
 *  @author Johnny Willemsen  <jwillemsen@remedy.nl>
 */
//=============================================================================

#ifndef ACE_OS_INCLUDE_SYS_OS_SYSCTL_H
#define ACE_OS_INCLUDE_SYS_OS_SYSCTL_H

#include /**/ "ace/pre.h"

#include /**/ "ace/config-lite.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if !defined (ACE_LACKS_SYS_SYSCTL_H)
  #if defined(__linux__) /* linux */
#include /**/ <linux/sysctl.h>
  #elif  defined(__APPLE__) /* MacOS */
#include /***/  <sys/sysctl.h>
  #elif defined(__unix__) /* other unix like */
#include  /**/  <sys/sysctl.h>
  #endif
#endif /* !ACE_LACKS_SYS_SYSCTL_H */

#include /**/ "ace/post.h"
#endif /* ACE_OS_INCLUDE_SYS_OS_SYSCTL_H */