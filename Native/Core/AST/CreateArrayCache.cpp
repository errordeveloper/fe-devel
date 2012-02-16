/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/AST/CreateArrayCache.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/IntegerAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/ArrayProducerAdapter.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Base/Util/SimpleString.h>
#include <Fabric/Base/Exception.h>

#include <llvm/Module.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( CreateArrayCache );
    
    RC::ConstHandle<CreateArrayCache> CreateArrayCache::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &input
      )
    {
      return new CreateArrayCache( location, input );
    }
    
    CreateArrayCache::CreateArrayCache(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &input
      )
      : Expr( location )
      , m_input( input )
    {
    }
    
    void CreateArrayCache::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_input->appendJSON( jsonObjectEncoder.makeMember( "input" ), includeLocation );
    }
    
    void CreateArrayCache::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_input->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType CreateArrayCache::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      return m_input->getExprType( basicBlockBuilder );
    }
    
    CG::ExprValue CreateArrayCache::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "cannot be used as l-values" );
      
      RC::Handle<CG::Context> context = basicBlockBuilder.getContext();
      llvm::LLVMContext &llvmContext = context->getLLVMContext();

      CG::ExprType inputExprType = m_input->getExprType( basicBlockBuilder );
      if ( !RT::isArrayProducer( inputExprType.getAdapter()->getType() ) )
        throw CG::Error( getLocation(), "input must be an array producer" );

      RC::ConstHandle<CG::ArrayProducerAdapter> inputArrayProducerAdapter = RC::ConstHandle<CG::ArrayProducerAdapter>::StaticCast( inputExprType.getAdapter() );
      CG::ExprValue inputExprRValue = m_input->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
      llvm::Value *resultLValue = inputArrayProducerAdapter->llvmAlloca( basicBlockBuilder, "result" );
      inputArrayProducerAdapter->llvmInit( basicBlockBuilder, resultLValue );
      basicBlockBuilder.getScope().put(
        CG::VariableSymbol::Create(
          CG::ExprValue(
            inputArrayProducerAdapter,
            CG::USAGE_LVALUE,
            context,
            resultLValue
            )
          )
        );
    
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( inputArrayProducerAdapter->llvmLType( context ) );
      argTypes.push_back( inputArrayProducerAdapter->llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateArrayCache", funcType );

      basicBlockBuilder->CreateCall2(
        func,
        inputExprRValue.getValue(),
        resultLValue
        );

      return CG::ExprValue(
        inputArrayProducerAdapter,
        CG::USAGE_RVALUE,
        context,
        inputArrayProducerAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue )
        );
    }
  }
}
