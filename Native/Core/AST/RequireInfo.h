/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_REQUIRE_INFO_H
#define _FABRIC_AST_REQUIRE_INFO_H

#include <Fabric/Core/CG/Location.h>

#include <string>
#include <map>

namespace Fabric
{
  namespace AST
  {
    class RequireNameToLocationMap : public std::map< std::string, CG::Location >
    {
    public:
    
      RequireNameToLocationMap()
      {
      }
    };
  }
}

#endif //_FABRIC_AST_REQUIRE_INFO_H
