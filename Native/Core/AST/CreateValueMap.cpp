/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/CreateValueMap.h>
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
    FABRIC_AST_NODE_IMPL( CreateValueMap );
    
    RC::ConstHandle<CreateValueMap> CreateValueMap::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &input,
      std::string const &operatorName,
      RC::ConstHandle<Expr> const &shared
      )
    {
      return new CreateValueMap( location, input, operatorName, shared );
    }
    
    CreateValueMap::CreateValueMap(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &input,
      std::string const &operatorName,
      RC::ConstHandle<Expr> const &shared
      )
      : Expr( location )
      , m_input( input )
      , m_operatorName( operatorName )
      , m_shared( shared )
    {
    }
    
    void CreateValueMap::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_input->appendJSON( jsonObjectEncoder.makeMember( "input" ), includeLocation );
      jsonObjectEncoder.makeMember( "operatorName" ).makeString( m_operatorName );
      m_shared->appendJSON( jsonObjectEncoder.makeMember( "shared" ), includeLocation );
    }
    
    void CreateValueMap::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_input->registerTypes( cgManager, diagnostics );
      if ( m_shared )
        m_shared->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType CreateValueMap::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Symbol> operatorSymbol = basicBlockBuilder.getScope().get( m_operatorName );
      if ( !operatorSymbol )
        throw CG::Error( getLocation(), _(m_operatorName) + ": operator not found" );
      if ( !operatorSymbol->isPencil() )
        throw CG::Error( getLocation(), _(m_operatorName) + ": not an operator" );
      RC::ConstHandle<CG::PencilSymbol> pencil = RC::ConstHandle<CG::PencilSymbol>::StaticCast( operatorSymbol );
      CG::Function const *function = pencil->getUniqueFunction( getLocation(), "operator " + _(m_operatorName) );
      std::vector<CG::FunctionParam> const &operatorParams = function->getParams();
      RC::ConstHandle<CG::Adapter> outputAdapter = operatorParams[1].getAdapter();
      RC::ConstHandle<CG::ValueProducerAdapter> outputValueProducerAdapter = basicBlockBuilder.getManager()->getValueProducerOf( outputAdapter );
      return CG::ExprType( outputValueProducerAdapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue CreateValueMap::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "cannot be used as l-values" );
      
      RC::Handle<CG::Context> context = basicBlockBuilder.getContext();
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
            
      RC::ConstHandle<CG::Symbol> operatorSymbol = basicBlockBuilder.getScope().get( m_operatorName );
      if ( !operatorSymbol )
        throw CG::Error( getLocation(), _(m_operatorName) + ": operator not found" );
      if ( !operatorSymbol->isPencil() )
        throw CG::Error( getLocation(), _(m_operatorName) + ": not an operator" );
      RC::ConstHandle<CG::PencilSymbol> pencil = RC::ConstHandle<CG::PencilSymbol>::StaticCast( operatorSymbol );
      CG::Function const *function = pencil->getUniqueFunction( getLocation(), "operator " + _(m_operatorName) );
      std::vector<CG::FunctionParam> const &operatorParams = function->getParams();

      CG::ExprType inputExprType = m_input->getExprType( basicBlockBuilder );
      if ( !RT::isValueProducer( inputExprType.getAdapter()->getType() ) )
        throw CG::Error( getLocation(), "input must be a value producer" );
      RC::ConstHandle<CG::ValueProducerAdapter> inputValueProducerAdapter = RC::ConstHandle<CG::ValueProducerAdapter>::StaticCast( inputExprType.getAdapter() );
      RC::ConstHandle<CG::Adapter> inputAdapter = inputValueProducerAdapter->getValueAdapter();
      CG::ExprValue inputExprRValue = m_input->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );

      RC::ConstHandle<CG::Adapter> outputAdapter = operatorParams[1].getAdapter();
      RC::ConstHandle<CG::ValueProducerAdapter> outputValueProducerAdapter = basicBlockBuilder.getManager()->getValueProducerOf( outputAdapter );
      llvm::Value *resultLValue = outputValueProducerAdapter->llvmAlloca( basicBlockBuilder, "result" );
      outputValueProducerAdapter->llvmInit( basicBlockBuilder, resultLValue );
      basicBlockBuilder.getScope().put(
        CG::VariableSymbol::Create(
          CG::ExprValue(
            outputValueProducerAdapter,
            CG::USAGE_LVALUE,
            context,
            resultLValue
            )
          )
        );
      
      if ( !m_shared )
      {
        if ( operatorParams.size() != 2
          || operatorParams[0].getAdapter() != inputAdapter
          || operatorParams[0].getUsage() != CG::USAGE_RVALUE
          || operatorParams[1].getUsage() != CG::USAGE_LVALUE
          )
          throw CG::Error( getLocation(), "operator must take parameters: in " + inputAdapter->getUserName() + " input, io OutputType output" );
        
        std::vector<llvm::Type const *> argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( inputValueProducerAdapter->llvmLType( context ) );
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( outputValueProducerAdapter->llvmLType( context ) );
        llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
        llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateValueMap_2", funcType );
        
        basicBlockBuilder->CreateCall4(
          func,
          basicBlockBuilder->CreateBitCast(
            function->getLLVMFunction(),
            llvm::Type::getInt8PtrTy( llvmContext )
            ),
          inputExprRValue.getValue(),
          outputValueProducerAdapter->llvmAdapterPtr( basicBlockBuilder ),
          resultLValue
          );
      }
      else
      {
        CG::ExprType sharedExprType = m_shared->getExprType( basicBlockBuilder );
        if ( !RT::isValueProducer( sharedExprType.getAdapter()->getType() ) )
          throw CG::Error( getLocation(), "shared must be a value producer" );
        RC::ConstHandle<CG::ValueProducerAdapter> sharedValueProducerAdapter = RC::ConstHandle<CG::ValueProducerAdapter>::StaticCast( sharedExprType.getAdapter() );
        RC::ConstHandle<CG::Adapter> sharedAdapter = sharedValueProducerAdapter->getValueAdapter();
        CG::ExprValue sharedExprRValue = m_shared->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
        
        if ( operatorParams.size() != 3
          || operatorParams[0].getAdapter() != inputAdapter
          || operatorParams[0].getUsage() != CG::USAGE_RVALUE
          || operatorParams[1].getUsage() != CG::USAGE_LVALUE
          || operatorParams[2].getAdapter() != sharedAdapter
          || operatorParams[2].getUsage() != CG::USAGE_RVALUE
          )
          throw CG::Error( getLocation(), "operator must take parameters: in " + inputAdapter->getUserName() + " input, io OutputType output, in " + sharedAdapter->getUserName() + " shared" );
      
        std::vector<llvm::Type const *> argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( inputValueProducerAdapter->llvmLType( context ) );
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( sharedValueProducerAdapter->llvmLType( context ) );
        argTypes.push_back( outputValueProducerAdapter->llvmLType( context ) );
        llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
        llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateValueMap_3", funcType );
        
        basicBlockBuilder->CreateCall5(
          func,
          basicBlockBuilder->CreateBitCast(
            function->getLLVMFunction(),
            llvm::Type::getInt8PtrTy( llvmContext )
            ),
          inputExprRValue.getValue(),
          outputValueProducerAdapter->llvmAdapterPtr( basicBlockBuilder ),
          sharedExprRValue.getValue(),
          resultLValue
          );
      }
      
      return CG::ExprValue(
        outputValueProducerAdapter,
        CG::USAGE_RVALUE,
        context,
        outputValueProducerAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue )
        );
    }
  }
}
