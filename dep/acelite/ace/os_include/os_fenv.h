// -*- C++ -*-

//=============================================================================
/**
 *  @file    os_fenv.h
 *
 *  floating-point environment
 *
 *  $Id: os_fenv.h 97827 2014-08-02 17:34:32Z johnnyw $
 *
 *  @author Don Hinton <dhinton@dresystems.com>
 *  @author This code was originally in various places including ace/OS.h.
 */
//=============================================================================

#ifndef ACE_OS_INCLUDE_OS_FENV_H
#define ACE_OS_INCLUDE_OS_FENV_H

#include /**/ "ace/pre.h"

#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if !defined (ACE_LACKS_FENV_H)
# include /**/ <fenv.h>
#endif /* !ACE_LACKS_FENV_H */

#include /**/ "ace/post.h"
#endif /* ACE_OS_INCLUDE_OS_FENV_H */
