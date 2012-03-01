/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "LogCollector.h"

namespace Fabric
{
  namespace MT
  {
    LogCollector::LogCollector( bool buffering )
      : m_bufferMutex( "LogCollector" )
    {
      // [pzion 20110315] This has the side effect of assigning the
      // role of main thread to the calling thread
      getPrefix_CRIT();
    }
    
    LogCollector::~LogCollector()
    {
    }
    
    void LogCollector::add( char const *data, size_t length )
    {
      Util::Mutex::Lock lock( m_bufferMutex );
      std::string prefixedString = getPrefix_CRIT() + std::string( data, length );
      FABRIC_LOG( prefixedString );
      if ( !m_buffer.empty() )
        m_buffer += "\n";
      m_buffer += prefixedString;
    }
    
    std::string LogCollector::getPrefix_CRIT()
    {
      ThreadID threadID = MT::getCurrentThreadID();
      ThreadPrefixMap::const_iterator it = m_threadPrefixes.find( threadID );
      if ( it == m_threadPrefixes.end() )
      {
        char buf[16];
        if ( m_threadPrefixes.empty() )
          strcpy( buf, "[MT] " );
        else snprintf( buf, 16, "[%02u] ", (unsigned)m_threadPrefixes.size() );
        it = m_threadPrefixes.insert( ThreadPrefixMap::value_type( threadID, buf ) ).first;
      }
      return it->second;
    }
    
    void LogCollector::flush()
    {
      Util::Mutex::Lock lock( m_bufferMutex );
      if ( !m_buffer.empty() )
      {
        logString( m_buffer.data(), m_buffer.length() );
        m_buffer.clear();
      }
    }
    
    Util::TLSVar<LogCollector *> tlsLogCollector;
  };
};
