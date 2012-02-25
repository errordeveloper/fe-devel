/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_ASSIGNED_VAR_DECL_H
#define _FABRIC_AST_ASSIGNED_VAR_DECL_H

#include <Fabric/Core/AST/VarDecl.h>

namespace Fabric
{
  namespace AST
  {
    class Expr;
    
    class AssignedVarDecl : public VarDecl
    {
      FABRIC_AST_NODE_DECL( AssignedVarDecl );
      
    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<AssignedVarDecl> Create(
        CG::Location const &location,
        std::string const &name,
        std::string const &arrayModifier,
        RC::ConstHandle<Expr> initialExpr
        );

      virtual void registerTypes( std::string const &baseType, RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual void llvmCompileToBuilder( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
     
    protected:
    
      AssignedVarDecl(
        CG::Location const &location,
        std::string const &name,
        std::string const &arrayModifier,
        RC::ConstHandle<Expr> const &initialExpr
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<Expr> m_initialExpr;
    };
  };
};

#endif //_FABRIC_AST_VAR_DECL_H
