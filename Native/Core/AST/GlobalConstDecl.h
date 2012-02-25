/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_GLOBAL_CONST_DECL_H
#define _FABRIC_AST_GLOBAL_CONST_DECL_H

#include <Fabric/Core/AST/Global.h>

namespace Fabric
{
  namespace AST
  {
    class ConstDecl;
    
    class GlobalConstDecl : public Global
    {
      FABRIC_AST_NODE_DECL( GlobalConstDecl );

    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<GlobalConstDecl> Create(
        CG::Location const &location,
        RC::ConstHandle<ConstDecl> const &constDecl
        )
      {
        return new GlobalConstDecl( location, constDecl );
      }
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const;
      
    protected:
    
      GlobalConstDecl(
        CG::Location const &location,
        RC::ConstHandle<ConstDecl> const &constDecl
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<ConstDecl> m_constDecl;
    };
  };
};

#endif //_FABRIC_AST_GLOBAL_CONST_DECL_H
