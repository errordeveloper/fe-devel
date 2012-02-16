/*
 *  AssignOp.cpp
 *  Fabric
 *
 *  Created by Halfdan Ingvarsson on 11-01-10.
 *  Copyright 2011 Fabric Technologies Inc. All rights reserved.
 *
 */

#include <Fabric/Core/AST/AssignOp.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Core/CG/PencilSymbol.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( AssignOp );
    
    RC::ConstHandle<AssignOp> AssignOp::Create( CG::Location const &location, CG::AssignOpType assignOpType, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right )
    {
      return new AssignOp( location, assignOpType, left, right );
    }
    
    AssignOp::AssignOp( CG::Location const &location, CG::AssignOpType assignOpType, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right )
      : Expr( location )
      , m_assignOpType( assignOpType )
      , m_left( left )
      , m_right( right )
    {
    }
    
    void AssignOp::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "type" ).makeString( assignOpCodeName( m_assignOpType ) );
      m_left->appendJSON( jsonObjectEncoder.makeMember( "lhs" ), includeLocation );
      m_right->appendJSON( jsonObjectEncoder.makeMember( "rhs" ), includeLocation );
    }
    
    void AssignOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_left->registerTypes( cgManager, diagnostics );
      m_right->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType AssignOp::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      CG::ExprType lhsExprType = m_left->getExprType( basicBlockBuilder );
      if ( lhsExprType.getUsage() != CG::USAGE_LVALUE )
        throw CG::Error( getLocation(), "cannot be assigned to" );
      lhsExprType.getAdapter()->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return lhsExprType;
    }
    
    CG::ExprValue AssignOp::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      try
      {
        CG::ExprValue lhsExprValue = m_left->buildExprValue( basicBlockBuilder, CG::USAGE_LVALUE, "cannot be assigned to" );

        RC::ConstHandle<CG::Adapter> adapter = lhsExprValue.getAdapter();
        CG::ExprValue rhsExprValue = m_right->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
          
        if ( m_assignOpType == CG::ASSIGN_OP )
        {
          llvm::Value *rhsCastedRValue = adapter->llvmCast( basicBlockBuilder, rhsExprValue );
          adapter->llvmAssign( basicBlockBuilder, lhsExprValue.getValue(), rhsCastedRValue );
          return lhsExprValue;
        }

        CG::Function const *function = basicBlockBuilder.getModuleBuilder().maybeGetPreciseFunction(
          CG::AssignOpPencilKey( adapter, m_assignOpType ),
          CG::ExprTypeVector(
            lhsExprValue.getExprType(),
            rhsExprValue.getExprType()
            )
          );
        if ( function )
        {
          function->llvmCreateCall( basicBlockBuilder, lhsExprValue, rhsExprValue );
          return lhsExprValue;
        }
        
        // [pzion 20110202] Fall back on binOp + simple assignOp composition
        CG::BinOpType binOpType = CG::binOpForAssignOp( m_assignOpType );   
        function = basicBlockBuilder.getModuleBuilder().getFunction(
          getLocation(),
          CG::BinOpPencilKey( binOpType ),
          CG::ExprTypeVector(
            lhsExprValue.getExprType(),
            rhsExprValue.getExprType()
            ),
          CG::BinOpQueryDesc(
            binOpType,
            lhsExprValue.getAdapter(),
            rhsExprValue.getAdapter()
            )
          );
        CG::ExprValue binOpResultExprValue = function->llvmCreateCall( basicBlockBuilder, lhsExprValue, rhsExprValue );
        llvm::Value *rhsCastedRValue = adapter->llvmCast( basicBlockBuilder, binOpResultExprValue );
        adapter->llvmAssign( basicBlockBuilder, lhsExprValue.getValue(), rhsCastedRValue );
        return lhsExprValue;
      }
      catch ( CG::Error e )
      {
        throw "assignment: " + e;
      }
    }
  };
};
