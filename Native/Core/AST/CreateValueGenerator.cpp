/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/AST/CreateValueGenerator.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/IntegerAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/CG/ValueProducerAdapter.h>
#include <Fabric/Core/MR/ValueGeneratorOperator.h>
#include <Fabric/Base/Util/SimpleString.h>

#include <llvm/Module.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( CreateValueGenerator );
    
    RC::ConstHandle<CreateValueGenerator> CreateValueGenerator::Create(
      CG::Location const &location,
      std::string const &operatorName,
      RC::ConstHandle<Expr> const &shared
      )
    {
      return new CreateValueGenerator( location, operatorName, shared );
    }
    
    CreateValueGenerator::CreateValueGenerator(
      CG::Location const &location,
      std::string const &operatorName,
      RC::ConstHandle<Expr> const &shared
      )
      : Expr( location )
      , m_operatorName( operatorName )
      , m_shared( shared )
    {
    }
    
    void CreateValueGenerator::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "operatorName" ).makeString( m_operatorName );
      m_shared->appendJSON( jsonObjectEncoder.makeMember( "shared" ), includeLocation );
    }
    
    void CreateValueGenerator::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      if ( m_shared )
        m_shared->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType CreateValueGenerator::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Symbol> operatorSymbol = basicBlockBuilder.getScope().get( m_operatorName );
      if ( !operatorSymbol )
        throw CG::Error( getLocation(), _(m_operatorName) + ": operator not found" );
      if ( !operatorSymbol->isPencil() )
        throw CG::Error( getLocation(), _(m_operatorName) + ": not an operator" );
      RC::ConstHandle<CG::PencilSymbol> pencil = RC::ConstHandle<CG::PencilSymbol>::StaticCast( operatorSymbol );
      CG::Function const *function = pencil->getUniqueFunction( getLocation(), "operator " + _(m_operatorName) );
      std::vector<CG::FunctionParam> const &operatorParams = function->getParams();
      RC::ConstHandle<CG::Adapter> outputAdapter = operatorParams[0].getAdapter();
      RC::ConstHandle<CG::ValueProducerAdapter> outputValueProducerAdapter = basicBlockBuilder.getManager()->getValueProducerOf( outputAdapter );
      return CG::ExprType( outputValueProducerAdapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue CreateValueGenerator::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "cannot be used as l-values" );
      
      RC::Handle<CG::Context> context = basicBlockBuilder.getContext();
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
      RC::ConstHandle<CG::SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      
      RC::ConstHandle<CG::Symbol> operatorSymbol = basicBlockBuilder.getScope().get( m_operatorName );
      if ( !operatorSymbol )
        throw CG::Error( getLocation(), _(m_operatorName) + ": operator not found" );
      if ( !operatorSymbol->isPencil() )
        throw CG::Error( getLocation(), _(m_operatorName) + ": not an operator" );
      RC::ConstHandle<CG::PencilSymbol> pencil = RC::ConstHandle<CG::PencilSymbol>::StaticCast( operatorSymbol );
      CG::Function const *function = pencil->getUniqueFunction( getLocation(), "operator " + _(m_operatorName) );
      std::vector<CG::FunctionParam> const &operatorParams = function->getParams();
      if ( operatorParams.size() < 1 )
        throw MR::ValueGeneratorOperator::GetPrototypeException();

      if ( operatorParams[0].getUsage() != CG::USAGE_LVALUE )
        throw MR::ValueGeneratorOperator::GetPrototypeException();
      RC::ConstHandle<CG::Adapter> outputAdapter = operatorParams[0].getAdapter();
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
        std::vector<llvm::Type const *> argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( sizeAdapter->llvmRType( context ) );
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( outputValueProducerAdapter->llvmLType( context ) );
        llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
        llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateValueGenerator_1", funcType );
        
        basicBlockBuilder->CreateCall4(
          func,
          basicBlockBuilder->CreateBitCast(
            function->getLLVMFunction(),
            llvm::Type::getInt8PtrTy( llvmContext )
            ),
          sizeAdapter->llvmConst( context, operatorParams.size() ),
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
        
        if ( operatorParams.size() != 2
          || operatorParams[1].getAdapter() != sharedAdapter
          || operatorParams[1].getUsage() != CG::USAGE_RVALUE
          )
          throw MR::ValueGeneratorOperator::GetPrototypeException();
      
        std::vector<llvm::Type const *> argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( sizeAdapter->llvmRType( context ) );
        argTypes.push_back( sharedValueProducerAdapter->llvmLType( context ) );
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( outputValueProducerAdapter->llvmLType( context ) );
        llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
        llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateValueGenerator_2", funcType );
        
        basicBlockBuilder->CreateCall5(
          func,
          basicBlockBuilder->CreateBitCast(
            function->getLLVMFunction(),
            llvm::Type::getInt8PtrTy( llvmContext )
            ),
          sizeAdapter->llvmConst( context, operatorParams.size() ),
          sharedExprRValue.getValue(),
          outputValueProducerAdapter->llvmAdapterPtr( basicBlockBuilder ),
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
