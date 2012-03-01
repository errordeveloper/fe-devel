/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_DEBUG_H
#define _FABRIC_UTIL_DEBUG_H

#include <Fabric/Base/Util/Log.h>
#include <Fabric/Core/Util/AutoPtr.h>
#include <Fabric/Core/Util/TLS.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Core/Util/Timer.h>

#include <stdarg.h>
#include <string.h>

#if !defined(FABRIC_OS_WINDOWS)
# include <sys/time.h>
# include <pthread.h>
#endif

#include <string>

#if defined(FABRIC_BUILD_RELEASE)
inline bool __eat_string( const char *fmt, ... ) { return false; }
inline bool __eat_string( std::string const &string ) { return false; }
#endif

#if defined(FABRIC_BUILD_RELEASE)
# define FABRIC_DEBUG_LOG( format, ... )
#else //defined(FABRIC_BUILD_DEBUG)
# define FABRIC_DEBUG_LOG( format, ... ) \
  FABRIC_LOG( format, ##__VA_ARGS__)
#endif

namespace Fabric
{
  namespace Util
  {
    class Trace
    {
    public:

      Trace( const char *module )
      {
        strncpy( m_module, module, sizeof( m_module ) );
#if defined(FABRIC_OS_WINDOWS)
        LARGE_INTEGER   pcFreq;
        ::QueryPerformanceFrequency( &pcFreq );
        m_pcFreq = float(pcFreq.QuadPart)/1000.f;
#endif
      }

      void init( const char *format, ... )
      {
        m_desc.resize( 1024 );
        
        va_list argList;
        
        va_start( argList, format );
        vsnprintf( &m_desc[0], 1024, format, argList );
        va_end( argList );      
        
        note( "begin" );
        m_indent = m_indent + 1;
      }
      
      void init( std::string const &string )
      {
        m_desc = string;
        note( "begin" );
        m_indent = m_indent + 1;
      }

      ~Trace()
      {
        float elapsed = m_timer.getElapsedMS();
        m_indent = m_indent - 1;
        note( "end (elapsed=%.3fms)", elapsed );
      }
      
      void note( const char *format, ... )
      {
        va_list argList;
        std::string string;
        string.resize( 1024 );
        
        va_start( argList, format );
        vsnprintf( &string[0], 1024, format, argList );
        va_end( argList );
        
        note( string );
      }
      
      void note( std::string const &string )
      {
        if ( !m_indentStrings )
        {
          m_indentStrings = new char *[256];
          for ( int i=0; i<256; ++i )
            m_indentStrings[i] = 0;
        }
        if ( !m_indentStrings[m_indent] )
        {
          m_indentStrings[m_indent] = new char[m_indent+1];
          for ( unsigned i=0; i<m_indent; ++i )
            m_indentStrings[m_indent][i] = ' ';
          m_indentStrings[m_indent][m_indent] = '\0';
        }

        size_t threadId;
#if defined(FABRIC_OS_WINDOWS)
        threadId = (size_t)::GetCurrentThreadId();
#else
        threadId = (size_t)pthread_self();
#endif

        FABRIC_LOG( "[%x] %s%s::%s: %s\n", (unsigned)threadId, m_indentStrings[m_indent], m_module, m_desc.c_str(), string.c_str() );
      }
      
      void timestamp()
      {
#if defined(FABRIC_OS_WINDOWS)
        LARGE_INTEGER   pcNow;
        ::QueryPerformanceCounter( &pcNow );
        float now = float(pcNow.QuadPart)/m_pcFreq;
        note( "Timestamp is %g ms", now );
#else
        struct timeval tv;
        gettimeofday( &tv, NULL );
        note( "Timestamp is %09u.%06u", (unsigned)tv.tv_sec, (unsigned)tv.tv_usec );
#endif
      }
      
      void dumpBuffer( unsigned size, void const *data );
      
    private:

      static TLSVar<unsigned> m_indent;
      static char **m_indentStrings;
      std::string m_desc;
      char m_module[80];
      Timer m_timer;
#if defined(FABRIC_OS_WINDOWS)
      float m_pcFreq;
#endif
    };
  };
};

#if defined(FABRIC_BUILD_RELEASE)

# define _FABRIC_TRACE( module ) false && __eat_string
# define _FABRIC_TRACE_NOTE false && __eat_string
# define _FABRIC_TRACE_TIMESTAMP()
# define _FABRIC_TRACE_DUMP_BUFFER( size, data )

#else //defined(FABRIC_BUILD_DEBUG)

# define _FABRIC_TRACE( _m_ ) \
  Fabric::Util::AutoPtr<Fabric::Util::Trace> _trace; if ( ::Fabric::_m_::gDebugEnabled ) (_trace = new Fabric::Util::Trace( #_m_ ))->init

# define _FABRIC_TRACE_NOTE \
  if ( _trace ) _trace->note

# define _FABRIC_TRACE_TIMESTAMP() \
  if ( _trace )_trace->timestamp()

# define _FABRIC_TRACE_DUMP_BUFFER( size, data ) \
  if ( _trace ) _trace->dumpBuffer( (size), (data) )

#endif

#if defined(FABRIC_BUILD_DEBUG)
namespace Fabric
{
  namespace Util
  {
    extern bool gDebugEnabled;
  };
};
#endif

#define FABRIC_UTIL_TRACE _FABRIC_TRACE( Util )
#define FABRIC_UTIL_TRACE_NOTE _FABRIC_TRACE_NOTE
#define FABRIC_UTIL_TRACE_DUMP_BUFFER( size, data ) _FABRIC_TRACE_DUMP_BUFFER( size, data )

#endif //_FABRIC_UTIL_DEBUG_H
