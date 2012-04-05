/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/CreateArrayGenerator.h>
#include <Fabric/Core/MR/ArrayGeneratorOperator.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/IntegerAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/ArrayProducerAdapter.h>
#include <Fabric/Core/CG/ValueProducerAdapter.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Base/Util/SimpleString.h>
#include <Fabric/Base/Exception.h>

#include <llvm/Module.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( CreateArrayGenerator );
    
    RC::ConstHandle<CreateArrayGenerator> CreateArrayGenerator::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &count,
      std::string const &operatorName,
      RC::ConstHandle<Expr> const &shared
      )
    {
      return new CreateArrayGenerator( location, count, operatorName, shared );
    }
    
    CreateArrayGenerator::CreateArrayGenerator(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &count,
      std::string const &operatorName,
      RC::ConstHandle<Expr> const &shared
      )
      : Expr( location )
      , m_count( count )
      , m_operatorName( operatorName )
      , m_shared( shared )
    {
    }
    
    void CreateArrayGenerator::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_count->appendJSON( jsonObjectEncoder.makeMember( "count" ), includeLocation );
      jsonObjectEncoder.makeMember( "operatorName" ).makeString( m_operatorName );
      m_shared->appendJSON( jsonObjectEncoder.makeMember( "shared" ), includeLocation );
    }
    
    void CreateArrayGenerator::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_count->registerTypes( cgManager, diagnostics );
      if ( m_shared )
        m_shared->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType CreateArrayGenerator::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Symbol> operatorSymbol = basicBlockBuilder.getScope().get( m_operatorName );
      if ( !operatorSymbol )
        throw CG::Error( getLocation(), _(m_operatorName) + ": operator not found" );
      if ( !operatorSymbol->isPencil() )
        throw CG::Error( getLocation(), _(m_operatorName) + ": not an operator" );
      RC::ConstHandle<CG::PencilSymbol> pencil = RC::ConstHandle<CG::PencilSymbol>::StaticCast( operatorSymbol );
      CG::Function const *function = pencil->getUniqueFunction( getLocation(), "operator " + _(m_operatorName) );
      std::vector<CG::FunctionParam> const &operatorParams = function->getParams();
      if ( operatorParams.size() < 1 )
        throw MR::ArrayGeneratorOperator::GetPrototypeException();
      RC::ConstHandle<CG::Adapter> outputAdapter = operatorParams[0].getAdapter();
      RC::ConstHandle<CG::ArrayProducerAdapter> outputArrayProducerAdapter = basicBlockBuilder.getManager()->getArrayProducerOf( outputAdapter );
      return CG::ExprType( outputArrayProducerAdapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue CreateArrayGenerator::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
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
        throw MR::ArrayGeneratorOperator::GetPrototypeException();

      CG::ExprType countExprType = m_count->getExprType( basicBlockBuilder );
      if ( !RT::isValueProducer( countExprType.getAdapter()->getType() ) )
        throw CG::Error( getLocation(), "count must be a value producer" );
      RC::ConstHandle<CG::ValueProducerAdapter> countValueProducerAdapter = RC::ConstHandle<CG::ValueProducerAdapter>::StaticCast( countExprType.getAdapter() );
      RC::ConstHandle<CG::Adapter> countAdapter = countValueProducerAdapter->getValueAdapter();
      if ( !countAdapter->isEquivalentTo( sizeAdapter ) )
        throw CG::Error( getLocation(), "count value type must be 'Size'" );
      CG::ExprValue countValueProducerExprValue = m_count->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
        
      if ( operatorParams[0].getUsage() != CG::USAGE_LVALUE )
        throw CG::Error( getLocation(), "operator value parameter must be an 'io' parameter" );
      RC::ConstHandle<CG::Adapter> outputAdapter = operatorParams[0].getAdapter();
      RC::ConstHandle<CG::ArrayProducerAdapter> outputArrayProducerAdapter = basicBlockBuilder.getManager()->getArrayProducerOf( outputAdapter );
      llvm::Value *resultLValue = outputArrayProducerAdapter->llvmAlloca( basicBlockBuilder, "result" );
      outputArrayProducerAdapter->llvmInit( basicBlockBuilder, resultLValue );
      basicBlockBuilder.getScope().put(
        CG::VariableSymbol::Create(
          CG::ExprValue(
            outputArrayProducerAdapter,
            CG::USAGE_LVALUE,
            context,
            resultLValue
            )
          )
        );
    
      bool needCall = true;
      if ( operatorParams.size() >= 2 )
      {
        if ( !operatorParams[1].getAdapter()->isEquivalentTo( sizeAdapter ) )
          throw CG::Error( getLocation(), "operator index parameter type (" + operatorParams[1].getAdapter()->getUserName() + ") must be 'Index'" );
        if ( operatorParams[1].getUsage() != CG::USAGE_RVALUE )
          throw CG::Error( getLocation(), "operator index parameter must be an 'in' parameter" );
          
        if ( operatorParams.size() >= 3 )
        {
          if ( !operatorParams[2].getAdapter()->isEquivalentTo( sizeAdapter ) )
            throw CG::Error( getLocation(), "operator count parameter type (" + operatorParams[2].getAdapter()->getUserName() + ") must be 'Size'" );
          if ( operatorParams[2].getUsage() != CG::USAGE_RVALUE )
            throw CG::Error( getLocation(), "operator count parameter must be an 'in' parameter" );
            
          if ( operatorParams.size() >= 4 )
          {
            if ( operatorParams.size() > 4 )
              throw MR::ArrayGeneratorOperator::GetPrototypeException();
              
            if ( !m_shared )
              throw CG::Error( getLocation(), "operator takes a shared value but no shared value is provided" );
              
            CG::ExprType sharedExprType = m_shared->getExprType( basicBlockBuilder );
            if ( !RT::isValueProducer( sharedExprType.getAdapter()->getType() ) )
              throw CG::Error( getLocation(), "shared value must be a value producer" );
            RC::ConstHandle<CG::ValueProducerAdapter> sharedValueProducerAdapter = RC::ConstHandle<CG::ValueProducerAdapter>::StaticCast( sharedExprType.getAdapter() );
            RC::ConstHandle<CG::Adapter> sharedAdapter = sharedValueProducerAdapter->getValueAdapter();

            if ( operatorParams[3].getAdapter() != sharedAdapter )
              throw CG::Error( getLocation(), "operator shared value parameter type (" + operatorParams[3].getAdapter()->getUserName() + ") does not match shared value type (" + sharedAdapter->getUserName() + ")" );
            if ( operatorParams[3].getUsage() != CG::USAGE_RVALUE )
              throw CG::Error( getLocation(), "operator shared value parameter must be an 'in' parameter" );

            CG::ExprValue sharedExprRValue = m_shared->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );

            std::vector<llvm::Type *> argTypes;
            argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) ); // function
            argTypes.push_back( sizeAdapter->llvmRType( context ) ); // numParams
            argTypes.push_back( countValueProducerAdapter->llvmLType( context ) ); // count value producer
            argTypes.push_back( sharedValueProducerAdapter->llvmLType( context ) ); // shared value producer
            argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) ); // output array producer adapter
            argTypes.push_back( outputArrayProducerAdapter->llvmLType( context ) ); // output array producer
            llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
            llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateArrayGenerator_4", funcType );
            
            std::vector<llvm::Value *> args;
            args.push_back( basicBlockBuilder->CreateBitCast(
              function->getLLVMFunction(),
              llvm::Type::getInt8PtrTy( llvmContext )
              ) );
            args.push_back( sizeAdapter->llvmConst( context, operatorParams.size() ) );
            args.push_back( countValueProducerExprValue.getValue() );
            args.push_back( sharedExprRValue.getValue() );
            args.push_back( outputArrayProducerAdapter->llvmAdapterPtr( basicBlockBuilder ) );
            args.push_back( resultLValue );
            basicBlockBuilder->CreateCall( func, args );
            
            needCall = false;
          }
        }
      }
      
      if ( needCall )
      {
        std::vector<llvm::Type *> argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) ); // function
        argTypes.push_back( sizeAdapter->llvmRType( context ) ); // numParams
        argTypes.push_back( countValueProducerAdapter->llvmLType( context ) ); // count value producer
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) ); // output array producer adapter
        argTypes.push_back( outputArrayProducerAdapter->llvmLType( context ) ); // output array producer
        llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
        llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateArrayGenerator_3", funcType );
        
        std::vector<llvm::Value *> args;
        args.push_back( basicBlockBuilder->CreateBitCast(
          function->getLLVMFunction(),
          llvm::Type::getInt8PtrTy( llvmContext )
          ) );
        args.push_back( sizeAdapter->llvmConst( context, operatorParams.size() ) );
        args.push_back( countValueProducerExprValue.getValue() );
        args.push_back( outputArrayProducerAdapter->llvmAdapterPtr( basicBlockBuilder ) );
        args.push_back( resultLValue );
        basicBlockBuilder->CreateCall( func, args );
      }

      return CG::ExprValue(
        outputArrayProducerAdapter,
        CG::USAGE_RVALUE,
        context,
        outputArrayProducerAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue )
        );
    }
  }
}
