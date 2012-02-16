/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#include <Fabric/Core/AST/AssignedVarDecl.h>
#include <Fabric/Core/AST/Expr.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( AssignedVarDecl );
    
    RC::ConstHandle<AssignedVarDecl> AssignedVarDecl::Create(
      CG::Location const &location,
      std::string const &name,
      std::string const &arrayModifier,
      RC::ConstHandle<Expr> initialExpr
      )
    {
      return new AssignedVarDecl( location, name, arrayModifier, initialExpr );
    }

    AssignedVarDecl::AssignedVarDecl(
      CG::Location const &location,
      std::string const &name,
      std::string const &arrayModifier,
      RC::ConstHandle<Expr> const &initialExpr
      )
      : VarDecl( location, name, arrayModifier )
      , m_initialExpr( initialExpr )
    {
    }
    
    void AssignedVarDecl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      VarDecl::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_initialExpr->appendJSON( jsonObjectEncoder.makeMember( "initialValue" ), includeLocation );
    }
    
    void AssignedVarDecl::registerTypes( std::string const &baseType, RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      VarDecl::registerTypes( baseType, cgManager, diagnostics );
      m_initialExpr->registerTypes( cgManager, diagnostics );
    }

    void AssignedVarDecl::llvmCompileToBuilder( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      try
      {
        CG::ExprValue result = VarDecl::llvmAllocateVariable( baseType, basicBlockBuilder, diagnostics );
        
        CG::ExprValue initialExprExprValue( basicBlockBuilder.getContext() );
        try
        {
          initialExprExprValue = m_initialExpr->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, "cannot be an l-value" );
        }
        catch ( CG::Error e )
        {
          addError( diagnostics, e );
        }
        
        if( initialExprExprValue.isValid() )
        {
          llvm::Value *initialExprCastedRValue = result.getAdapter()->llvmCast( basicBlockBuilder, initialExprExprValue );
          result.getAdapter()->llvmAssign( basicBlockBuilder, result.getValue(), initialExprCastedRValue );
        }
        else addError( diagnostics, "Invalid expression for variable assignment" );
      }
      catch ( CG::Error e )
      {
        throw e;
      }
      catch ( Exception e )
      {
        throw CG::Error( getLocation(), e );
      }
    }
  };
};
