/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_JSON_COMMAND_CHANNEL_H
#define _FABRIC_JSON_COMMAND_CHANNEL_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <string>
#include <vector>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace JSON
  {
    class Value;
    
    class CommandChannel : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      virtual void jsonNotify(
        std::vector<std::string> const &src,
        char const *cmdData,
        size_t cmdLength,
        Util::SimpleString const *argJSON = 0
        ) = 0;
    };
  };
};

#endif //_FABRIC_JSON_COMMAND_CHANNEL_H
