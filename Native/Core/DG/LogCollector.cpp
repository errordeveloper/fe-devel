/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/LogCollector.h>
#include <Fabric/Core/DG/Context.h>

namespace Fabric
{
  namespace DG
  {
    RC::Handle<LogCollector> LogCollector::Create( Context *context )
    {
      return new LogCollector( context );
    }
    
    LogCollector::LogCollector( Context *context )
      : m_context( context )
    {
    }
    
    LogCollector::~LogCollector()
    {
    }
    
    void LogCollector::logString( char const *data, size_t length )
    {
      std::vector<std::string> src;
      src.push_back( "DG" );
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jg.makeString( data, length );
      }
      m_context->jsonNotify( src, "log", 3, &json );
    }
  };
};
