/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CLI_LOG_COLLECTOR_H
#define _FABRIC_CLI_LOG_COLLECTOR_H

#include <Fabric/Core/MT/LogCollector.h>

namespace Fabric
{
  namespace CLI
  {
    class LogCollector : public MT::LogCollector
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<LogCollector> Create();
    
    protected:
    
      LogCollector();
      ~LogCollector();
      
      virtual void logString( char const *data, size_t length );
      
    private:
    
    };
  };
};

#endif //_FABRIC_CLI_LOG_COLLECTOR_H
