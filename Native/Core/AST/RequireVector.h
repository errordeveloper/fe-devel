/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_REQUIRE_VECTOR_H
#define _FABRIC_AST_REQUIRE_VECTOR_H

#include <Fabric/Base/RC/Vector.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <string>
#include <set>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace AST
  {
    class Require;
    class RequireNameToLocationMap;
    
    class RequireVector : public RC::Vector< RC::ConstHandle<Require> >
    {
    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<RequireVector> Create( RC::ConstHandle<Require> const &first = 0, RC::ConstHandle<RequireVector> const &remaining = 0 );

      void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
      void collectRequires( RequireNameToLocationMap &uses ) const;

    protected:
    
      RequireVector();
    };
  }
}

#endif //_FABRIC_AST_REQUIRE_VECTOR_H
