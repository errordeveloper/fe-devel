/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_MUTEX_H
#define _FABRIC_UTIL_MUTEX_H

#include <Fabric/Base/Config.h>
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Core/Util/Debug.h>

#if defined(FABRIC_POSIX)
# include <pthread.h>
#elif defined(FABRIC_WIN32) 
# include <windows.h>
#else
# error "missing FABRIC_OS_... definition"
#endif

namespace Fabric
{
  namespace Util
  {
    class Mutex
    {
      friend class Cond;
      
    public:
    
      Mutex( char const *name )
        : m_name( name )
      {
#if defined(FABRIC_OS_WINDOWS) 
        ::InitializeCriticalSection( &m_cs );
#else
        pthread_mutexattr_t   attr;
        pthread_mutexattr_init( &attr );
        pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
                
        FABRIC_VERIFY( pthread_mutex_init( &m_mutex, &attr ) == 0 );
        
        pthread_mutexattr_destroy( &attr );
#endif
      }
      
      ~Mutex()
      {
#if defined(FABRIC_OS_WINDOWS) 
        ::DeleteCriticalSection( &m_cs );
#else
        FABRIC_VERIFY( pthread_mutex_destroy( &m_mutex ) == 0 );
#endif
      }
      
      class Lock
      {
      public:
      
        Lock( Mutex &mutex )
          : m_mutex( mutex )
        {
          m_mutex.acquire();
        }
        
        ~Lock()
        {
          m_mutex.release();
        }
        
      private:
      
        Mutex &m_mutex;
      };
      
      void acquire()
      {
#if defined(FABRIC_OS_WINDOWS) 
        ::EnterCriticalSection( &m_cs );
#else
        if ( pthread_mutex_lock( &m_mutex ) != 0 )
          throw Exception( "pthread_mutex_lock(): unknown failure" );
#endif
      }
      
      void release()
      {
#if defined(FABRIC_OS_WINDOWS) 
        ::LeaveCriticalSection( &m_cs );
#else
        if ( pthread_mutex_unlock( &m_mutex ) != 0 )
          throw Exception( "pthread_mutex_unlock(): unknown failure" );
#endif
      }

    private:

      char const *m_name;
#if defined(FABRIC_OS_WINDOWS) 
      CRITICAL_SECTION m_cs;
#else
      pthread_mutex_t m_mutex;
#endif
    };
  };
};

#endif //_FABRIC_UTIL_MUTEX_H
