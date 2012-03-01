/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MT_LOG_COLLECTOR_H
#define _FABRIC_MT_LOG_COLLECTOR_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Core/MT/Util.h>
#include <Fabric/Core/Util/AutoSet.h>
#include <Fabric/Core/Util/Mutex.h>
#include <Fabric/Core/Util/UnorderedMap.h>

namespace Fabric
{
  namespace MT
  {
    class LogCollector : public RC::Object
    {
      typedef Util::UnorderedMap< MT::ThreadID, std::string > ThreadPrefixMap;
      
    public:
      REPORT_RC_LEAKS
    
      void add( char const *data, size_t length );
      void add( char const *cString )
      {
        add( cString, strlen(cString) );
      }
      void add( std::string const &string )
      {
        add( string.data(), string.length() );
      }
      
      void flush();
      
    protected:
    
      LogCollector( bool buffering = true );
      virtual ~LogCollector();
      
      std::string getPrefix_CRIT();

      virtual void logString( char const *data, size_t length ) = 0;
      
    private:
    
      Util::Mutex m_bufferMutex;
      std::string m_buffer;
      ThreadPrefixMap m_threadPrefixes;
    };
    
    typedef Util::TLSVar<LogCollector *> TLSLogCollector;
    
    extern TLSLogCollector tlsLogCollector;
    
    class TLSLogCollectorAutoSet : public Util::AutoSet<LogCollector *>
    {
    public:
    
      TLSLogCollectorAutoSet( RC::Handle<LogCollector> const &logCollector )
        : Util::AutoSet<LogCollector *>::AutoSet( tlsLogCollector, logCollector.ptr() )
      {
      }
    };
  };
};

#endif //_FABRIC_MT_LOG_COLLECTOR_H
