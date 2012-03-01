/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "NotOp.h"
#include <Fabric/Core/CG/BooleanAdapter.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( NotOp );
    
    NotOp::NotOp( CG::Location const &location, RC::ConstHandle<Expr> const &child )
      : Expr( location )
      , m_child( child )
    {
    }
    
    void NotOp::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_child->appendJSON( jsonObjectEncoder.makeMember( "child" ), includeLocation );
    }
    
    void NotOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_child->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType NotOp::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = basicBlockBuilder.getManager()->getBooleanAdapter();
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return CG::ExprType( adapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue NotOp::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      try
      {
        if ( usage == CG::USAGE_LVALUE )
          throw CG::Error( getLocation(), "result of logical not "+lValueErrorDesc );
          
        CG::ExprValue childExprValue = m_child->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
        RC::ConstHandle< CG::Adapter > booleanAdapter = basicBlockBuilder.getManager()->getBooleanAdapter();
        llvm::Value *booleanRValue = booleanAdapter->llvmCast( basicBlockBuilder, childExprValue );
        llvm::Value *notBooleanRValue = basicBlockBuilder->CreateNot( booleanRValue );
        return CG::ExprValue( booleanAdapter, CG::USAGE_RVALUE, basicBlockBuilder.getContext(), notBooleanRValue );
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
