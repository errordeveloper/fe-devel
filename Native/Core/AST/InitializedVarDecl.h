/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_INITIALIZED_VAR_DECL_H
#define _FABRIC_AST_INITIALIZED_VAR_DECL_H

#include <Fabric/Core/AST/VarDecl.h>
#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class ExprVector;
    
    class InitializedVarDecl: public VarDecl
    {
      FABRIC_AST_NODE_DECL( InitializedVarDecl );

    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<InitializedVarDecl> Create(
        CG::Location const &location,
        std::string const &name,
        std::string const &arrayModifier,
        RC::ConstHandle<ExprVector> const &args
        );

      virtual void registerTypes( std::string const &baseType, RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;

      virtual void llvmCompileToBuilder( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
     
    protected:
    
      InitializedVarDecl(
        CG::Location const &location,
        std::string const &name,
        std::string const &arrayModifier,
        RC::ConstHandle<ExprVector> const &args
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<ExprVector> m_args;
    };
  };
};

#endif //_FABRIC_AST_INITIALIZED_VAR_DECL_H
