/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_COND_H
#define _FABRIC_UTIL_COND_H

// [pzion 20110713] Windows implementation taken from http://www1.cse.wustl.edu/~schmidt/win32-cv-1.html

#include <Fabric/Base/Config.h>
#include <Fabric/Base/Exception.h>

#if defined(FABRIC_POSIX)
# include <pthread.h>
#elif defined(FABRIC_WIN32) 
# include <windows.h>
#else
# error "missing FABRIC_PLATFORM_... definition"
#endif

namespace Fabric
{
  namespace Util
  {
    class Cond
    {
    public:
    
      Cond()
      {
#if defined(FABRIC_POSIX)
        if ( pthread_cond_init( &m_posixCond, 0 ) != 0 )
          throw Exception( "pthread_cond_init(): unknown failure" );
#elif defined(FABRIC_WIN32)
        ::InitializeCriticalSection( &m_windowsCS );
        m_windowsGeneration = 0;
        m_windowsWaitCount = 0;
        m_windowsSignalCount = 0;
        m_windowsEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( !m_windowsEvent )
          throw Exception( "CreateEvent(): unknown failure" );
#else
# error "missing FABRIC_PLATFORM_... definition"
#endif
      }
      
      ~Cond()
      {
#if defined(FABRIC_POSIX)
        if ( pthread_cond_destroy( &m_posixCond ) != 0 )
          throw Exception( "pthread_cond_destroy(): unknown failure" );
#elif defined(FABRIC_WIN32)
        if ( !::CloseHandle( m_windowsEvent ) )
          throw Exception( "CloseHandle(): unknown failure" );
        ::DeleteCriticalSection( &m_windowsCS );
#else
# error "missing FABRIC_PLATFORM_... definition"
#endif
      }
      
      void wait( Mutex &mutex )
      {
#if defined(FABRIC_POSIX)
        if ( pthread_cond_wait( &m_posixCond, &mutex.m_mutex ) != 0 )
          throw Exception( "pthread_cond_wait(): unknown failure" );
#elif defined(FABRIC_WIN32)
        ::EnterCriticalSection( &m_windowsCS);
        size_t myGeneration = m_windowsGeneration;
        ++m_windowsWaitCount;
        ::LeaveCriticalSection( &m_windowsCS);

        ::LeaveCriticalSection( &mutex.m_cs );
        for (;;)
        {
          ::WaitForSingleObject( m_windowsEvent, INFINITE );

          ::EnterCriticalSection( &m_windowsCS );
          bool waitDone = m_windowsSignalCount > 0 && myGeneration != m_windowsGeneration;
          ::LeaveCriticalSection( &m_windowsCS );

          if ( waitDone )
            break;
        }
        ::EnterCriticalSection( &mutex.m_cs );

        ::EnterCriticalSection( &m_windowsCS );
        --m_windowsWaitCount;
        if ( --m_windowsSignalCount == 0 )
          ::ResetEvent( m_windowsEvent );
        ::LeaveCriticalSection( &m_windowsCS );
#else
# error "missing FABRIC_PLATFORM_... definition"
#endif
      }

      void signal()
      {
#if defined(FABRIC_POSIX)
        if ( pthread_cond_signal( &m_posixCond ) != 0 )
          throw Exception( "pthread_cond_signal(): unknown failure" );
#elif defined(FABRIC_WIN32)
        ::EnterCriticalSection( &m_windowsCS );
        if ( m_windowsWaitCount > m_windowsSignalCount )
        {
          ::SetEvent( m_windowsEvent );
          ++m_windowsSignalCount;
          ++m_windowsGeneration;
        }
        ::LeaveCriticalSection( &m_windowsCS );
#else
# error "missing FABRIC_PLATFORM_... definition"
#endif
      }

      void broadcast()
      {
#if defined(FABRIC_POSIX)
        if ( pthread_cond_broadcast( &m_posixCond ) != 0 )
          throw Exception( "pthread_cond_broadcast(): unknown failure" );
#elif defined(FABRIC_WIN32)
        ::EnterCriticalSection( &m_windowsCS );
        if ( m_windowsWaitCount > 0 )
        {  
          ::SetEvent( m_windowsEvent );
          m_windowsSignalCount = m_windowsWaitCount;
          ++m_windowsGeneration;
        }
        ::LeaveCriticalSection( &m_windowsCS );
#else
# error "missing FABRIC_PLATFORM_... definition"
#endif
      }

    private:
    
#if defined(FABRIC_POSIX)
      pthread_cond_t m_posixCond;
#elif defined(FABRIC_WIN32)
      CRITICAL_SECTION m_windowsCS;
      size_t m_windowsGeneration;
      size_t m_windowsWaitCount;
      size_t m_windowsSignalCount;
      HANDLE m_windowsEvent;
#else
# error "missing FABRIC_PLATFORM_... definition"
#endif
    };
  };
};

#endif //_FABRIC_UTIL_COND_H
