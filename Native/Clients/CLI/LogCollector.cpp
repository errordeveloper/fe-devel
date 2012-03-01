/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "LogCollector.h"

namespace Fabric
{
  namespace CLI
  {
    RC::Handle<LogCollector> LogCollector::Create()
    {
      return new LogCollector();
    }
    
    LogCollector::LogCollector()
    {
    }
    
    LogCollector::~LogCollector()
    {
    }
    
    void LogCollector::logString( char const *data, size_t length )
    {
    }
  };
};
