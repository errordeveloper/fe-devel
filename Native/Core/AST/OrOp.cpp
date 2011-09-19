/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#include "OrOp.h"
#include <Fabric/Core/CG/BooleanAdapter.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( OrOp );
    
    OrOp::OrOp( CG::Location const &location, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right )
      : Expr( location )
      , m_left( left )
      , m_right( right )
    {
    }
    
    void OrOp::appendJSONMembers( Util::JSONObjectGenerator const &jsonObjectGenerator, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectGenerator, includeLocation );
      m_left->appendJSON( jsonObjectGenerator.makeMember( "lhs" ), includeLocation );
      m_right->appendJSON( jsonObjectGenerator.makeMember( "rhs" ), includeLocation );
    }
    
    void OrOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_left->registerTypes( cgManager, diagnostics );
      m_right->registerTypes( cgManager, diagnostics );
    }
    
    RC::ConstHandle<CG::Adapter> OrOp::getType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> lhsType = m_left->getType( basicBlockBuilder );
      if ( lhsType )
        lhsType->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      RC::ConstHandle<CG::Adapter> rhsType = m_right->getType( basicBlockBuilder );
      if ( rhsType )
        rhsType->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
  
      // The true/false value types need to be "equivalent". We'll cast into whoever wins the
      // casting competition, or fail if they can't.
      RC::ConstHandle<RT::Desc> castType = basicBlockBuilder.getStrongerTypeOrNone( lhsType->getDesc(), rhsType->getDesc() );
      if ( !castType )
        throw CG::Error( getLocation(), "types " + _(lhsType->getUserName()) + " and " + _(rhsType->getUserName()) + " are unrelated" );
      return lhsType->getManager()->getAdapter( castType );
    }
    
    CG::ExprValue OrOp::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw CG::Error( getLocation(), "the result of a boolean 'or' operation cannot be an l-value" );
      else usage = CG::USAGE_RVALUE;
      
      RC::ConstHandle<CG::BooleanAdapter> booleanAdapter = basicBlockBuilder.getManager()->getBooleanAdapter();
      
      llvm::BasicBlock *lhsTrueBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "orLHSTrue" );
      llvm::BasicBlock *lhsFalseBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "orLHSFalse" );
      llvm::BasicBlock *mergeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "orMerge" );

      CG::ExprValue result( basicBlockBuilder.getContext() );
      CG::ExprValue lhsExprValue = m_left->buildExprValue( basicBlockBuilder, usage, lValueErrorDesc );
      llvm::Value *lhsBooleanRValue = booleanAdapter->llvmCast( basicBlockBuilder, lhsExprValue );
      basicBlockBuilder->CreateCondBr( lhsBooleanRValue, lhsTrueBB, lhsFalseBB );

      basicBlockBuilder->SetInsertPoint( lhsFalseBB );
      CG::ExprValue rhsExprValue = m_right->buildExprValue( basicBlockBuilder, usage, lValueErrorDesc );
      RC::ConstHandle<CG::Adapter> castAdapter = getType( basicBlockBuilder );
      
      llvm::Value *rhsCastedRValue = castAdapter->llvmCast( basicBlockBuilder, rhsExprValue );
      llvm::BasicBlock *lhsFalsePredBB = basicBlockBuilder->GetInsertBlock();
      basicBlockBuilder->CreateBr( mergeBB );
      
      basicBlockBuilder->SetInsertPoint( lhsTrueBB );
      llvm::Value *lhsCastedRValue = castAdapter->llvmCast( basicBlockBuilder, lhsExprValue );
      llvm::BasicBlock *lhsTruePredBB = basicBlockBuilder->GetInsertBlock();
      basicBlockBuilder->CreateBr( mergeBB );

      basicBlockBuilder->SetInsertPoint( mergeBB );
      llvm::PHINode *phi = basicBlockBuilder->CreatePHI( castAdapter->llvmRType( basicBlockBuilder.getContext() ) );
      phi->addIncoming( lhsCastedRValue, lhsTruePredBB );
      phi->addIncoming( rhsCastedRValue, lhsFalsePredBB );
      return CG::ExprValue( castAdapter, usage, basicBlockBuilder.getContext(), phi );
    }
  };
};
