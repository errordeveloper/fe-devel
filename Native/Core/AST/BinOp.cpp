/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#include <Fabric/Core/AST/BinOp.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/OverloadNames.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/OpTypes.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( BinOp );
    
    RC::ConstHandle<BinOp> BinOp::Create( CG::Location const &location, CG::BinOpType binOpType, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right )
    {
      return new BinOp( location, binOpType, left, right );
    }

    BinOp::BinOp( CG::Location const &location, CG::BinOpType binOpType, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right )
      : Expr( location )
      , m_binOpType( binOpType )
      , m_left( left )
      , m_right( right )
    {
    }
    
    void BinOp::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "binOpType" ).makeString( CG::binOpUserName( m_binOpType ) );
      m_left->appendJSON( jsonObjectEncoder.makeMember( "lhs" ), includeLocation );
      m_right->appendJSON( jsonObjectEncoder.makeMember( "rhs" ), includeLocation );
    }
    
    RC::ConstHandle<CG::FunctionSymbol> BinOp::getFunctionSymbol( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      CG::ExprType lhsType = m_left->getExprType( basicBlockBuilder );
      lhsType.getAdapter()->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      CG::ExprType rhsType = m_right->getExprType( basicBlockBuilder );
      rhsType.getAdapter()->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      
      std::string functionName = CG::binOpOverloadName( m_binOpType, lhsType.getAdapter(), rhsType.getAdapter() );
      RC::ConstHandle<CG::FunctionSymbol> functionSymbol = basicBlockBuilder.maybeGetFunction( functionName );
      if ( functionSymbol )
        return functionSymbol;
      
      // [pzion 20110317] Fall back on stronger type
      
      RC::ConstHandle<RT::Desc> castDesc = basicBlockBuilder.getStrongerTypeOrNone( lhsType.getDesc(), rhsType.getDesc() );
      if ( castDesc )
      {
        RC::ConstHandle<CG::Adapter> castType = basicBlockBuilder.getManager()->getAdapter( castDesc );

        functionName = CG::binOpOverloadName( m_binOpType, castType, castType );
        functionSymbol = basicBlockBuilder.maybeGetFunction( functionName );
        if ( functionSymbol )
          return functionSymbol;
      }
      
      // 

      throw Exception( "binary operator " + _(CG::binOpUserName( m_binOpType )) + " not supported for types " + _(lhsType.getUserName()) + " and " + _(rhsType.getUserName()) );
    }
    
    void BinOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_left->registerTypes( cgManager, diagnostics );
      m_right->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType BinOp::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = getFunctionSymbol( basicBlockBuilder )->getReturnInfo().getAdapter();
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return CG::ExprType( adapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue BinOp::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      RC::ConstHandle< CG::FunctionSymbol > functionSymbol = getFunctionSymbol( basicBlockBuilder );
      std::vector<CG::FunctionParam> const &functionParams = functionSymbol->getParams();
        
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "the result of " + functionParams[0].getAdapter()->getUserName() + " " + CG::binOpUserName( m_binOpType ) + " " + functionParams[1].getAdapter()->getUserName() + " is not an l-value" );
      else usage = CG::USAGE_RVALUE;
      
      CG::ExprValue result( basicBlockBuilder.getContext() );
      CG::ExprValue lhsExprValue = m_left->buildExprValue( basicBlockBuilder, functionParams[0].getUsage(), lValueErrorDesc );
      CG::ExprValue rhsExprValue = m_right->buildExprValue( basicBlockBuilder, functionParams[1].getUsage(), lValueErrorDesc );
      try
      {
        result = functionSymbol->llvmCreateCall( basicBlockBuilder, lhsExprValue, rhsExprValue );
      }
      catch ( Exception e )
      {
        throw CG::Error( getLocation(), "in expression " + functionParams[0].getAdapter()->getUserName() + " " + CG::binOpUserName( m_binOpType ) + " " + functionParams[1].getAdapter()->getUserName() + ": " + e.getDesc() );
      }
      return result;
    }
  };
};
