/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_REQUIRE_H
#define _FABRIC_AST_REQUIRE_H

#include <Fabric/Core/AST/Node.h>

namespace Fabric
{
  namespace AST
  {
    class RequireNameToLocationMap;
    
    class Require : public Node
    {
      FABRIC_AST_NODE_DECL( Require );
      
    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<Require> Create(
        CG::Location const &location,
        std::string const &name
        );
      
      void collectRequires( RequireNameToLocationMap &uses ) const;
     
    protected:
    
      Require(
        CG::Location const &location,
        std::string const &name
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      std::string m_name;
    };
  }
}

#endif //_FABRIC_AST_REQUIRE_H
