/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_ASSERT_H
#define _FABRIC_ASSERT_H

#include <Fabric/Base/Config.h>
#include <Fabric/Base/Util/Log.h>

#if defined(FABRIC_BUILD_RELEASE)
# define FABRIC_ASSERT(condition)
# define FABRIC_ASSERT_TEXT(condition,text)
# define FABRIC_VERIFY(condition) (void)(condition)
#else //defined(FABRIC_BUILD_DEBUG)
# if defined( FABRIC_OS_WINDOWS )
#  include <assert.h>
#  define FABRIC_ASSERT_TEXT( condition, text ) \
  (void)( (!!(condition)) || (_wassert(_CRT_WIDE(text), _CRT_WIDE(__FILE__), __LINE__), 0) )
# define FABRIC_ASSERT(condition) \
  FABRIC_ASSERT_TEXT( condition, #condition )
# define FABRIC_VERIFY(condition) \
  FABRIC_ASSERT_TEXT( condition, #condition )
# else
#  include <stdlib.h>
#  define FABRIC_ASSERT_TEXT(condition,text) \
  do { \
    if ( !(condition) ) { \
      FABRIC_LOG( "ASSERTION FAILURE: \"%s\", file %s, line %d", text, __FILE__, __LINE__ ); \
      abort(); \
    } \
  } while(false)
# define FABRIC_ASSERT(condition) \
  FABRIC_ASSERT_TEXT( condition, "" #condition )
# define FABRIC_VERIFY(condition) \
  FABRIC_ASSERT_TEXT( condition, "" #condition )
# endif
#endif

#endif //_FABRIC_ASSERT_H
