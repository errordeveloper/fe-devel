/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/CreateConstValue.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/IntegerAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/ValueProducerAdapter.h>
#include <Fabric/Base/Util/SimpleString.h>

#include <llvm/Module.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( CreateConstValue );
    
    RC::ConstHandle<CreateConstValue> CreateConstValue::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &child
      )
    {
      return new CreateConstValue( location, child );
    }
    
    CreateConstValue::CreateConstValue(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &child
      )
      : Expr( location )
      , m_child( child )
    {
    }
    
    void CreateConstValue::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_child->appendJSON( jsonObjectEncoder.makeMember( "child" ), includeLocation );
    }
    
    void CreateConstValue::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_child->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType CreateConstValue::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      return CG::ExprType( basicBlockBuilder.getManager()->getValueProducerOf( m_child->getExprType( basicBlockBuilder ).getAdapter() ), CG::USAGE_RVALUE );
    }
    
    CG::ExprValue CreateConstValue::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "cannot be used as l-values" );
      
      CG::ExprType childExprType = m_child->getExprType( basicBlockBuilder );
      RC::ConstHandle<CG::ValueProducerAdapter> valueProducerAdapter = basicBlockBuilder.getManager()->getValueProducerOf( childExprType.getAdapter() );
      
      RC::Handle<CG::Context> context = basicBlockBuilder.getContext();
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
      
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
      argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
      argTypes.push_back( valueProducerAdapter->llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateConstValue", funcType );
      
      CG::ExprValue childExprRValue = m_child->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
      llvm::Value *childExprLValue = childExprType.getAdapter()->llvmRValueToLValue( basicBlockBuilder, childExprRValue.getValue() );
      llvm::Value *resultLValue = valueProducerAdapter->llvmAlloca( basicBlockBuilder, "result" );
      valueProducerAdapter->llvmInit( basicBlockBuilder, resultLValue );
      basicBlockBuilder.getScope().put(
        CG::VariableSymbol::Create(
          CG::ExprValue(
            valueProducerAdapter,
            CG::USAGE_LVALUE,
            context,
            resultLValue
            )
          )
        );
      
      basicBlockBuilder->CreateCall3(
        func,
        valueProducerAdapter->llvmAdapterPtr( basicBlockBuilder ),
        basicBlockBuilder->CreatePointerCast(
          childExprLValue,
          llvm::Type::getInt8PtrTy( llvmContext )
          ),
        resultLValue
        );
      
      return CG::ExprValue(
        valueProducerAdapter,
        CG::USAGE_RVALUE,
        context,
        valueProducerAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue )
        );
    }
  }
}
