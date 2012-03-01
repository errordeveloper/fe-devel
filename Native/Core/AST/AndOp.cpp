/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/AndOp.h>
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
    FABRIC_AST_NODE_IMPL( AndOp );
    
    RC::ConstHandle<AndOp> AndOp::Create( CG::Location const &location, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right )
    {
      return new AndOp( location, left, right );
    }

    AndOp::AndOp( CG::Location const &location, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right )
      : Expr( location )
      , m_left( left )
      , m_right( right )
    {
    }
    
    void AndOp::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_left->appendJSON( jsonObjectEncoder.makeMember( "lhs" ), includeLocation );
      m_right->appendJSON( jsonObjectEncoder.makeMember( "rhs" ), includeLocation );
    }
    
    CG::ExprType AndOp::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      CG::ExprType lhsType = m_left->getExprType( basicBlockBuilder );
      if ( lhsType )
        lhsType.getAdapter()->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      CG::ExprType rhsType = m_right->getExprType( basicBlockBuilder );
      if ( rhsType )
        rhsType.getAdapter()->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
  
      if ( lhsType && rhsType )
      {
        // The true/false value types need to be "equivalent". We'll cast into whoever wins the
        // casting competition, or fail if they can't.
        RC::ConstHandle<RT::Desc> castType = basicBlockBuilder.getStrongerTypeOrNone( lhsType.getDesc(), rhsType.getDesc() );
        if ( !castType )
          throw CG::Error( getLocation(), "types " + _(lhsType.getUserName()) + " and " + _(rhsType.getUserName()) + " are unrelated" );
        return CG::ExprType( basicBlockBuilder.getManager()->getAdapter( castType ), CG::USAGE_RVALUE );
      }
      else return CG::ExprType();
    }
    
    void AndOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_left->registerTypes( cgManager, diagnostics );
      m_right->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprValue AndOp::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw CG::Error( getLocation(), "the result of a boolean 'and' operation " + lValueErrorDesc );
      else usage = CG::USAGE_RVALUE;
      
      RC::ConstHandle<CG::BooleanAdapter> booleanAdapter = basicBlockBuilder.getManager()->getBooleanAdapter();
      CG::ExprType castExprType = getExprType( basicBlockBuilder );
      
      llvm::BasicBlock *lhsTrueBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "andLHSTrue" );
      llvm::BasicBlock *lhsFalseBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "andLHSFalse" );
      llvm::BasicBlock *mergeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "andMerge" );

      CG::ExprValue lhsExprValue = m_left->buildExprValue( basicBlockBuilder, usage, lValueErrorDesc );
      llvm::Value *lhsBooleanRValue = booleanAdapter->llvmCast( basicBlockBuilder, lhsExprValue );
      basicBlockBuilder->CreateCondBr( lhsBooleanRValue, lhsTrueBB, lhsFalseBB );

      basicBlockBuilder->SetInsertPoint( lhsTrueBB );
      CG::ExprValue rhsExprValue = m_right->buildExprValue( basicBlockBuilder, usage, lValueErrorDesc );
      
      llvm::Value *rhsCastedRValue = 0;
      if ( castExprType )
        rhsCastedRValue = castExprType.getAdapter()->llvmCast( basicBlockBuilder, rhsExprValue );
      llvm::BasicBlock *lhsTruePredBB = basicBlockBuilder->GetInsertBlock();
      basicBlockBuilder->CreateBr( mergeBB );
      
      basicBlockBuilder->SetInsertPoint( lhsFalseBB );
      llvm::Value *lhsCastedRValue = 0;
      if ( castExprType )
        lhsCastedRValue = castExprType.getAdapter()->llvmCast( basicBlockBuilder, lhsExprValue );
      llvm::BasicBlock *lhsFalsePredBB = basicBlockBuilder->GetInsertBlock();
      basicBlockBuilder->CreateBr( mergeBB );

      basicBlockBuilder->SetInsertPoint( mergeBB );
      if ( castExprType )
      {
        llvm::PHINode *phi = basicBlockBuilder->CreatePHI( castExprType.getAdapter()->llvmRType( basicBlockBuilder.getContext() ) );
        phi->addIncoming( rhsCastedRValue, lhsTruePredBB );
        phi->addIncoming( lhsCastedRValue, lhsFalsePredBB );
        return CG::ExprValue( castExprType.getAdapter(), usage, basicBlockBuilder.getContext(), phi );
      }
      else return CG::ExprValue( basicBlockBuilder.getContext() );
    }
  };
};
