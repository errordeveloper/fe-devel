/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/AST/CreateReduce.h>
#include <Fabric/Core/MR/ArrayMapOperator.h>
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
    FABRIC_AST_NODE_IMPL( CreateReduce );
    
    RC::ConstHandle<CreateReduce> CreateReduce::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &input,
      std::string const &operatorName,
      RC::ConstHandle<Expr> const &shared
      )
    {
      return new CreateReduce( location, input, operatorName, shared );
    }
    
    CreateReduce::CreateReduce(
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
    
    void CreateReduce::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_input->appendJSON( jsonObjectEncoder.makeMember( "input" ), includeLocation );
      jsonObjectEncoder.makeMember( "operatorName" ).makeString( m_operatorName );
      m_shared->appendJSON( jsonObjectEncoder.makeMember( "shared" ), includeLocation );
    }
    
    void CreateReduce::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_input->registerTypes( cgManager, diagnostics );
      if ( m_shared )
        m_shared->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType CreateReduce::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Symbol> operatorSymbol = basicBlockBuilder.getScope().get( m_operatorName );
      if ( !operatorSymbol )
        throw CG::Error( getLocation(), _(m_operatorName) + ": function not found" );
      if ( !operatorSymbol->isPencil() )
        throw CG::Error( getLocation(), _(m_operatorName) + ": not a function" );
      RC::ConstHandle<CG::PencilSymbol> operator_ = RC::ConstHandle<CG::PencilSymbol>::StaticCast( operatorSymbol );
      CG::Function const *function = operator_->getUniqueFunction( getLocation(), "operator " + _(m_operatorName) );
      
      std::vector<CG::FunctionParam> const &operatorParams = function->getParams();
      RC::ConstHandle<CG::Adapter> outputAdapter = operatorParams[1].getAdapter();
      RC::ConstHandle<CG::ValueProducerAdapter> outputValueProducerAdapter = basicBlockBuilder.getManager()->getValueProducerOf( outputAdapter );
      return CG::ExprType( outputValueProducerAdapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue CreateReduce::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
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
      RC::ConstHandle<CG::PencilSymbol> operator_ = RC::ConstHandle<CG::PencilSymbol>::StaticCast( operatorSymbol );
      CG::Function const *function = operator_->getUniqueFunction( getLocation(), "operator " + _(m_operatorName) );
      CG::ParamVector const &operatorParams = function->getParams();
      if ( operatorParams.size() < 2 )
        throw MR::ArrayMapOperator::GetPrototypeException();

      CG::ExprType inputExprType = m_input->getExprType( basicBlockBuilder );
      if ( !RT::isArrayProducer( inputExprType.getAdapter()->getType() ) )
        throw CG::Error( getLocation(), "input must be a value producer" );
      RC::ConstHandle<CG::ArrayProducerAdapter> inputArrayProducerAdapter = RC::ConstHandle<CG::ArrayProducerAdapter>::StaticCast( inputExprType.getAdapter() );
      RC::ConstHandle<CG::Adapter> inputAdapter = inputArrayProducerAdapter->getElementAdapter();
      if ( operatorParams[0].getAdapter() != inputAdapter )
        throw CG::Error( getLocation(), "operator input value parameter type (" + operatorParams[0].getAdapter()->getUserName() + ") does not match input array producer element type (" + inputAdapter->getUserName() + ")" );
      if ( operatorParams[0].getUsage() != CG::USAGE_RVALUE )
        throw CG::Error( getLocation(), "operator input value parameter must be an 'in' parameter" );
      CG::ExprValue inputExprRArray = m_input->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );

      if ( operatorParams[1].getUsage() != CG::USAGE_LVALUE )
        throw CG::Error( getLocation(), "operator output value parameter must be an 'io' parameter" );
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
    
      bool needCall = true;
      if ( operatorParams.size() >= 3 )
      {
        if ( operatorParams[2].getAdapter() != sizeAdapter )
          throw CG::Error( getLocation(), "operator index parameter type (" + operatorParams[2].getAdapter()->getUserName() + ") must be 'Size'" );
        if ( operatorParams[2].getUsage() != CG::USAGE_RVALUE )
          throw CG::Error( getLocation(), "operator index parameter must be an 'in' parameter" );
          
        if ( operatorParams.size() >= 4 )
        {
          if ( operatorParams[3].getAdapter() != sizeAdapter )
            throw CG::Error( getLocation(), "operator index parameter type (" + operatorParams[3].getAdapter()->getUserName() + ") must be 'Size'" );
          if ( operatorParams[3].getUsage() != CG::USAGE_RVALUE )
            throw CG::Error( getLocation(), "operator index parameter must be an 'in' parameter" );
            
          if ( operatorParams.size() >= 5 )
          {
            if ( operatorParams.size() > 5 )
              throw MR::ArrayMapOperator::GetPrototypeException();
              
            if ( !m_shared )
              throw CG::Error( getLocation(), "operator takes a shared value but no shared value is provided" );
              
            CG::ExprType sharedExprType = m_shared->getExprType( basicBlockBuilder );
            if ( !RT::isValueProducer( sharedExprType.getAdapter()->getType() ) )
              throw CG::Error( getLocation(), "shared value must be a value producer" );
            RC::ConstHandle<CG::ValueProducerAdapter> sharedValueProducerAdapter = RC::ConstHandle<CG::ValueProducerAdapter>::StaticCast( sharedExprType.getAdapter() );
            RC::ConstHandle<CG::Adapter> sharedAdapter = sharedValueProducerAdapter->getValueAdapter();

            if ( operatorParams[4].getAdapter() != sharedAdapter )
              throw CG::Error( getLocation(), "operator shared value parameter type (" + operatorParams[4].getAdapter()->getUserName() + ") does not match shared value type (" + sharedAdapter->getUserName() + ")" );
            if ( operatorParams[4].getUsage() != CG::USAGE_RVALUE )
              throw CG::Error( getLocation(), "operator shared value parameter must be an 'in' parameter" );

            CG::ExprValue sharedExprRValue = m_shared->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );

            std::vector<llvm::Type const *> argTypes;
            argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) ); // function
            argTypes.push_back( sizeAdapter->llvmRType( context ) ); // numParams
            argTypes.push_back( inputArrayProducerAdapter->llvmLType( context ) ); // input array producer
            argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) ); // output value producer adapter
            argTypes.push_back( sharedValueProducerAdapter->llvmLType( context ) ); // shared array producer
            argTypes.push_back( outputValueProducerAdapter->llvmLType( context ) ); // output array producer
            llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
            llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateReduce_5", funcType );
            
            std::vector<llvm::Value *> args;
            args.push_back( basicBlockBuilder->CreateBitCast(
              function->getLLVMFunction(),
              llvm::Type::getInt8PtrTy( llvmContext )
              ) );
            args.push_back( sizeAdapter->llvmConst( context, operatorParams.size() ) );
            args.push_back( inputExprRArray.getValue() );
            args.push_back( outputValueProducerAdapter->llvmAdapterPtr( basicBlockBuilder ) );
            args.push_back( sharedExprRValue.getValue() );
            args.push_back( resultLValue );
            basicBlockBuilder->CreateCall( func, args.begin(), args.end() );
            
            needCall = false;
          }
        }
      }
      
      if ( needCall )
      {
        std::vector<llvm::Type const *> argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) ); // function
        argTypes.push_back( sizeAdapter->llvmRType( context ) ); // numParams
        argTypes.push_back( inputArrayProducerAdapter->llvmLType( context ) ); // input array producer
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) ); // output value producer adapter
        argTypes.push_back( outputValueProducerAdapter->llvmLType( context ) ); // output array producer
        llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
        llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateReduce_4", funcType );
        
        std::vector<llvm::Value *> args;
        args.push_back( basicBlockBuilder->CreateBitCast(
          function->getLLVMFunction(),
          llvm::Type::getInt8PtrTy( llvmContext )
          ) );
        args.push_back( sizeAdapter->llvmConst( context, operatorParams.size() ) );
        args.push_back( inputExprRArray.getValue() );
        args.push_back( outputValueProducerAdapter->llvmAdapterPtr( basicBlockBuilder ) );
        args.push_back( resultLValue );
        basicBlockBuilder->CreateCall( func, args.begin(), args.end() );
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
