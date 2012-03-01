/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_REQUIRE_GLOBAL_H
#define _FABRIC_AST_REQUIRE_GLOBAL_H

#include <Fabric/Core/AST/Global.h>

namespace Fabric
{
  namespace CG
  {
    class Adapter;
    class Manager;
  };
  
  namespace AST
  {
    class RequireVector;
    class RequireInfoSet;
    
    class RequireGlobal : public Global
    {
      FABRIC_AST_NODE_DECL( RequireGlobal );
      
    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<RequireGlobal> Create(
        CG::Location const &location,
        RC::ConstHandle<RequireVector> const &useVector
        );
      
      virtual void collectRequires( RequireNameToLocationMap &uses ) const;
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      virtual void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const;
      
    protected:
    
      RequireGlobal(
        CG::Location const &location,
        RC::ConstHandle<RequireVector> const &useVector
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<RequireVector> m_requireVector;
    };
  }
}

#endif //_FABRIC_AST_REQUIRE_GLOBAL_H
