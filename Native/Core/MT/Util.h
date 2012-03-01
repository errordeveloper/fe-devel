/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MT_UTIL_H
#define _FABRIC_MT_UTIL_H

#include <Fabric/Base/Config.h>

#if defined(FABRIC_POSIX)
# include <pthread.h>
#elif defined(FABRIC_WIN32)
#include <windows.h>
#endif

namespace Fabric
{
  namespace MT
  {
    size_t getNumCores();

#if defined(FABRIC_POSIX)
    typedef pthread_t ThreadID;
#elif defined(FABRIC_WIN32)
    typedef DWORD ThreadID;
#else
# error "Unsupported platform"
#endif

    inline ThreadID getCurrentThreadID()
    {
#if defined(FABRIC_POSIX)
      return pthread_self();
#elif defined(FABRIC_WIN32)
      return (DWORD)::GetCurrentThreadId();
#else
# error "Unsupported platform"
#endif
    }
  };
};

#endif //_FABRIC_MT_UTIL_H
