/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#include <Fabric/Core/AST/SwitchStatement.h>
#include <Fabric/Core/AST/Expr.h>
#include <Fabric/Core/AST/CaseVector.h>
#include <Fabric/Core/AST/Case.h>
#include <Fabric/Core/AST/StatementVector.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/Function.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/BooleanAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/PencilSymbol.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( SwitchStatement );
    
    RC::ConstHandle<SwitchStatement> SwitchStatement::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &expr,
      RC::ConstHandle<CaseVector> const &cases
      )
    {
      return new SwitchStatement( location, expr, cases );
    }
    
    SwitchStatement::SwitchStatement(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &expr,
      RC::ConstHandle<CaseVector> const &cases
      )
      : Statement( location )
      , m_expr( expr )
      , m_cases( cases )
    {
    }
    
    void SwitchStatement::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Statement::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_expr->appendJSON( jsonObjectEncoder.makeMember( "expr" ), includeLocation );
      m_cases->appendJSON( jsonObjectEncoder.makeMember( "cases" ), includeLocation );
    }
    
    void SwitchStatement::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_expr->registerTypes( cgManager, diagnostics );
      m_cases->registerTypes( cgManager, diagnostics );
    }

    void SwitchStatement::llvmCompileToBuilder( CG::BasicBlockBuilder &parentBasicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      try
      {
        CG::FunctionBuilder &functionBuilder = parentBasicBlockBuilder.getFunctionBuilder();
        RC::ConstHandle<CG::BooleanAdapter> booleanAdapter = parentBasicBlockBuilder.getManager()->getBooleanAdapter();

        size_t numCases = m_cases->size();
        std::vector<llvm::BasicBlock *> caseBodyBBs;
        llvm::BasicBlock *caseDefaultBodyBB = 0;
        for ( size_t i=0; i<numCases; ++i )
        {
          llvm::BasicBlock *caseBodyBB = functionBuilder.createBasicBlock( "switchBody"+_(i+1) );
          caseBodyBBs.push_back( caseBodyBB );

          RC::ConstHandle<Case> case_ = m_cases->get(i);
          if ( !case_->getExpr() )
          {
            if ( caseDefaultBodyBB )
              throw CG::Error( case_->getLocation(), "cannot have more than one 'default' clause in a switch statement" );
            caseDefaultBodyBB = caseBodyBB;
          }
        }
        
        llvm::BasicBlock *doneBB = functionBuilder.createBasicBlock( "switchDone" );
        if ( !caseDefaultBodyBB )
          caseDefaultBodyBB = doneBB;
        
        std::vector<llvm::BasicBlock *> caseTestBBs;
        for ( size_t i=0; i<numCases; ++i )
        {
          RC::ConstHandle<Case> case_ = m_cases->get(i);
          if ( case_->getExpr() )
            caseTestBBs.push_back( functionBuilder.createBasicBlock( "switchTest"+_(i+1) ) );
        }
        caseTestBBs.push_back( caseDefaultBodyBB );

        CG::SwitchScope switchScope( parentBasicBlockBuilder.getScope(), doneBB );
        CG::BasicBlockBuilder basicBlockBuilder( parentBasicBlockBuilder, switchScope );
        
        CG::ExprValue exprValue = m_expr->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, "cannot be an l-value" );
        basicBlockBuilder->CreateBr( caseTestBBs[0] );
        
        size_t caseIndex = 0;
        for ( size_t i=1; i<caseTestBBs.size(); ++i )
        {
          basicBlockBuilder->SetInsertPoint( caseTestBBs[i-1] );

          RC::ConstHandle<Case> case_;
          for (;;)
          {
            case_ = m_cases->get(caseIndex);
            if ( case_->getExpr() )
              break;
            ++caseIndex;
          }
          
          CG::ExprValue caseExprValue = case_->getExpr()->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, "cannot be an l-value" );

          CG::Function const *function = basicBlockBuilder.getModuleBuilder().getFunction(
            getLocation(),
            CG::BinOpPencilKey( CG::BIN_OP_EQ ),
            CG::ExprTypeVector(
              exprValue.getExprType(),
              caseExprValue.getExprType()
              ),
            CG::BinOpQueryDesc(
              CG::BIN_OP_EQ,
              exprValue.getAdapter(),
              caseExprValue.getAdapter()
              )
            );
          CG::ExprValue cmpExprValue = function->llvmCreateCall( basicBlockBuilder, exprValue, caseExprValue );
          llvm::Value *cmpBooleanRValue = booleanAdapter->llvmCast( basicBlockBuilder, cmpExprValue );
          
          basicBlockBuilder->CreateCondBr( cmpBooleanRValue, caseBodyBBs[caseIndex], caseTestBBs[i] );
          
          ++caseIndex;
        }
        
        for ( size_t i=0; i<numCases; ++i )
        {
          RC::ConstHandle<Case> case_ = m_cases->get(i);

          basicBlockBuilder->SetInsertPoint( caseBodyBBs[i] );
          case_->getStatements()->llvmCompileToBuilder( basicBlockBuilder, diagnostics );
          if ( !basicBlockBuilder->GetInsertBlock()->getTerminator() )
            basicBlockBuilder->CreateBr( i+1 == numCases? doneBB: caseBodyBBs[i+1] );
        }
        basicBlockBuilder->SetInsertPoint( doneBB );
      }
      catch ( CG::Error e )
      {
        addError( diagnostics, e );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
    }
  };
};
