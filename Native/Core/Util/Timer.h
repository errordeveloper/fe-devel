/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_TIMER_H
#define _FABRIC_UTIL_TIMER_H

#include <Fabric/Base/Config.h>

#if defined(FABRIC_POSIX)
# include <sys/time.h>
#endif

#if defined(FABRIC_WIN32)
# include <windows.h>
#endif

namespace Fabric
{
  namespace Util
  {
    class Timer
    {
    public:

      Timer()
      {
#if defined(FABRIC_OS_WINDOWS)
        LARGE_INTEGER   pcFreq;
        ::QueryPerformanceFrequency( &pcFreq );
        m_pcFreq = float(pcFreq.QuadPart)/1000.f;
#endif
        reset();
      }

      void reset()
      {
#if defined(FABRIC_OS_WINDOWS)
        ::QueryPerformanceCounter( &m_pcBegin );
#else
        gettimeofday( &m_tvBegin, NULL );
#endif
      }

      float getElapsedMS( bool reset = false )
      {
        float elapsed;

#if defined(FABRIC_OS_WINDOWS)
        LARGE_INTEGER pcEnd;
        ::QueryPerformanceCounter( &pcEnd );
        elapsed = float(pcEnd.QuadPart - m_pcBegin.QuadPart)/m_pcFreq;
        if ( reset )
          m_pcBegin = pcEnd;
#else
        struct timeval tvEnd;
        gettimeofday( &tvEnd, NULL );
        elapsed = (float)(tvEnd.tv_sec-m_tvBegin.tv_sec)*1e3 + ((float)tvEnd.tv_usec - (float)m_tvBegin.tv_usec)*1e-3;
        if ( reset )
          m_tvBegin = tvEnd;
#endif
        
        return elapsed;
      }
      
    private:

#if defined(FABRIC_OS_WINDOWS)
      LARGE_INTEGER m_pcBegin;
      float m_pcFreq;
#else
      struct timeval m_tvBegin;
#endif
    };
  };
};

#endif //_FABRIC_UTIL_TIMER_H
