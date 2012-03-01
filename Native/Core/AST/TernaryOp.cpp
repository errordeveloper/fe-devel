/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "TernaryOp.h"
#include <Fabric/Core/CG/BooleanAdapter.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( TernaryOp );
    
    TernaryOp::TernaryOp(
      CG::Location const &location,
      CG::TernaryOpType opType, 
      RC::ConstHandle<Expr> const &left,
      RC::ConstHandle<Expr> const &middle,
      RC::ConstHandle<Expr> const &right
      )
      : Expr( location )
      , m_opType( opType )
      , m_left( left )
      , m_middle( middle )
      , m_right( right )
    {
    }
    
    void TernaryOp::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_left->appendJSON( jsonObjectEncoder.makeMember( "condExpr" ), includeLocation );
      m_middle->appendJSON( jsonObjectEncoder.makeMember( "trueExpr" ), includeLocation );
      m_right->appendJSON( jsonObjectEncoder.makeMember( "falseExpr" ), includeLocation );
    }
    
    void TernaryOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_left->registerTypes( cgManager, diagnostics );
      m_middle->registerTypes( cgManager, diagnostics );
      m_right->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType TernaryOp::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      CG::ExprType trueExprType = m_middle->getExprType( basicBlockBuilder );
      trueExprType.getAdapter()->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      CG::ExprType falseExprType = m_right->getExprType( basicBlockBuilder );
      falseExprType.getAdapter()->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      
      // The true/false value types need to be "equivalent". We'll cast into whoever wins the
      // casting competition, or fail if they can't.
      RC::ConstHandle<RT::Desc> castDesc = basicBlockBuilder.getStrongerTypeOrNone( trueExprType.getDesc(), falseExprType.getDesc() );
      if ( !castDesc )
        throw CG::Error( getLocation(), "types " + _(trueExprType.getUserName()) + " and " + _(falseExprType.getUserName()) + " are unrelated" );
      RC::ConstHandle<CG::Adapter> adapter = basicBlockBuilder.getManager()->getAdapter( castDesc );
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return CG::ExprType( adapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue TernaryOp::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw CG::Error( getLocation(), "the result of a ternary operation cannot be an l-value" );
      else usage = CG::USAGE_RVALUE;
      
      try
      {
        switch( m_opType )
        {
          case CG::TERNARY_OP_COND:
          {
            RC::ConstHandle<CG::BooleanAdapter> booleanAdapter = basicBlockBuilder.getManager()->getBooleanAdapter();
            CG::ExprType resultExprType = getExprType( basicBlockBuilder );
            RC::ConstHandle<CG::Adapter> resultAdapter = resultExprType.getAdapter();

            llvm::BasicBlock *trueBasicBlock = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "ternaryTrue" );
            llvm::BasicBlock *falseBasicBlock = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "ternaryFalse" );
            llvm::BasicBlock *mergeBasicBlock = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "ternaryMerge" );

            llvm::Value *resultLValue = resultAdapter->llvmAlloca( basicBlockBuilder, "ternaryResult" );
            resultAdapter->llvmInit( basicBlockBuilder, resultLValue );
            basicBlockBuilder.getScope().put(
              CG::VariableSymbol::Create(
                CG::ExprValue( resultAdapter, CG::USAGE_LVALUE, basicBlockBuilder.getContext(), resultLValue )
                )
              );

            llvm::Value *condValue = 0;
            CG::ExprValue exprExprValue = m_left->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
            condValue = booleanAdapter->llvmCast( basicBlockBuilder, exprExprValue );
            if( !condValue )
              return CG::ExprValue( basicBlockBuilder.getContext() );
            basicBlockBuilder->CreateCondBr( condValue, trueBasicBlock, falseBasicBlock );

            basicBlockBuilder->SetInsertPoint( trueBasicBlock );
            llvm::Value *trueRValue = 0;
            try
            {
              CG::Scope subScope( basicBlockBuilder.getScope() );
              CG::BasicBlockBuilder subBasicBlockBuilder( basicBlockBuilder, subScope );
              CG::ExprValue trueExprValue = m_middle->buildExprValue( subBasicBlockBuilder, usage, lValueErrorDesc );
              trueRValue = resultAdapter->llvmCast( subBasicBlockBuilder, trueExprValue );
              resultAdapter->llvmAssign( subBasicBlockBuilder, resultLValue, trueRValue );
              subScope.llvmUnwind( subBasicBlockBuilder );
            }
            catch ( Exception e )
            {
              CG::Error( getLocation(), e );
            }
            basicBlockBuilder->CreateBr( mergeBasicBlock );
            
            basicBlockBuilder->SetInsertPoint( falseBasicBlock );
            llvm::Value *falseRValue = 0;
            try
            {
              CG::Scope subScope( basicBlockBuilder.getScope() );
              CG::BasicBlockBuilder subBasicBlockBuilder( basicBlockBuilder, subScope );
              CG::ExprValue falseExprValue = m_right->buildExprValue( subBasicBlockBuilder, usage, lValueErrorDesc );
              falseRValue = resultAdapter->llvmCast( subBasicBlockBuilder, falseExprValue );
              resultAdapter->llvmAssign( subBasicBlockBuilder, resultLValue, falseRValue );
              subScope.llvmUnwind( subBasicBlockBuilder );
            }
            catch ( Exception e )
            {
              throw CG::Error( getLocation(), e );
            }
            basicBlockBuilder->CreateBr( mergeBasicBlock );
            
            // The merge path
            basicBlockBuilder->SetInsertPoint( mergeBasicBlock );
            llvm::Value *resultRValue = resultAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue );
            return CG::ExprValue( resultAdapter, CG::USAGE_RVALUE, basicBlockBuilder.getContext(), resultRValue );
          }
        }
      }
      catch ( CG::Error e )
      {
        throw "ternary operation: " + e;
      }
      catch ( Exception e )
      {
        throw CG::Error( getLocation(), "ternary operation: " + e );
      }
      return CG::ExprValue( basicBlockBuilder.getContext() );
    }
  };
};
