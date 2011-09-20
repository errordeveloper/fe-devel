/*
 *  TernaryOp.cpp
 *  
 *
 *  Created by Halfdan Ingvarsson on 11-01-11.
 *  Copyright 2011 Fabric Technologies Inc. All rights reserved.
 *
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
    
    void TernaryOp::appendJSONMembers( Util::JSONObjectGenerator const &jsonObjectGenerator, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectGenerator, includeLocation );
      m_left->appendJSON( jsonObjectGenerator.makeMember( "condExpr" ), includeLocation );
      m_middle->appendJSON( jsonObjectGenerator.makeMember( "trueExpr" ), includeLocation );
      m_right->appendJSON( jsonObjectGenerator.makeMember( "falseExpr" ), includeLocation );
    }
    
    void TernaryOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_left->registerTypes( cgManager, diagnostics );
      m_middle->registerTypes( cgManager, diagnostics );
      m_right->registerTypes( cgManager, diagnostics );
    }
    
    RC::ConstHandle<CG::Adapter> TernaryOp::getType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> trueType = m_middle->getType( basicBlockBuilder );
      trueType->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      RC::ConstHandle<CG::Adapter> falseType = m_right->getType( basicBlockBuilder );
      falseType->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      
      // The true/false value types need to be "equivalent". We'll cast into whoever wins the
      // casting competition, or fail if they can't.
      RC::ConstHandle<RT::Desc> castDesc = basicBlockBuilder.getStrongerTypeOrNone( trueType->getDesc(), falseType->getDesc() );
      if ( !castDesc )
        throw CG::Error( getLocation(), "types " + _(trueType->getUserName()) + " and " + _(falseType->getUserName()) + " are unrelated" );
      RC::ConstHandle<CG::Adapter> adapter = trueType->getManager()->getAdapter( castDesc );
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return adapter;
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

            llvm::BasicBlock *trueBasicBlock = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "cond_true" );
            llvm::BasicBlock *falseBasicBlock = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "cond_false" );
            llvm::BasicBlock *mergeBasicBlock = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "cond_merge" );

            llvm::Value *condValue = 0;
            CG::ExprValue exprExprValue = m_left->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
            condValue = booleanAdapter->llvmCast( basicBlockBuilder, exprExprValue );
            if( !condValue )
              return CG::ExprValue( basicBlockBuilder.getContext() );
            basicBlockBuilder->CreateCondBr( condValue, trueBasicBlock, falseBasicBlock );

            basicBlockBuilder->SetInsertPoint( trueBasicBlock );
            CG::ExprValue trueExprValue = m_middle->buildExprValue( basicBlockBuilder, usage, lValueErrorDesc );
            trueBasicBlock = basicBlockBuilder->GetInsertBlock();
            
            basicBlockBuilder->SetInsertPoint( falseBasicBlock );
            CG::ExprValue falseExprValue = m_right->buildExprValue( basicBlockBuilder, usage, lValueErrorDesc );
            falseBasicBlock = basicBlockBuilder->GetInsertBlock();

            RC::ConstHandle<CG::Adapter> castAdapter = getType( basicBlockBuilder );
            
            // The true value
            basicBlockBuilder->SetInsertPoint( trueBasicBlock );
            llvm::Value *trueRValue = 0;
            try
            {
              trueRValue = castAdapter->llvmCast( basicBlockBuilder, trueExprValue );
            }
            catch ( Exception e )
            {
              CG::Error( getLocation(), e );
            }
            basicBlockBuilder->CreateBr( mergeBasicBlock );
            
            // The false value
            basicBlockBuilder->SetInsertPoint( falseBasicBlock );
            llvm::Value *falseRValue = 0;
            try
            {
              falseRValue = castAdapter->llvmCast( basicBlockBuilder, falseExprValue );
            }
            catch ( Exception e )
            {
              throw CG::Error( getLocation(), e );
            }
            basicBlockBuilder->CreateBr( mergeBasicBlock );
            
            // The merge path
            basicBlockBuilder->SetInsertPoint( mergeBasicBlock );
            llvm::PHINode *phi = basicBlockBuilder->CreatePHI( castAdapter->llvmRType( basicBlockBuilder.getContext() ), "cond_phi" );
            phi->addIncoming( trueRValue, trueBasicBlock );
            phi->addIncoming( falseRValue, falseBasicBlock );
            
            return CG::ExprValue( castAdapter, usage, basicBlockBuilder.getContext(), phi );
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
