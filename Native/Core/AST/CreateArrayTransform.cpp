/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/CreateArrayTransform.h>
#include <Fabric/Core/MR/ArrayTransformOperator.h>
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
    FABRIC_AST_NODE_IMPL( CreateArrayTransform );
    
    RC::ConstHandle<CreateArrayTransform> CreateArrayTransform::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &input,
      std::string const &operatorName,
      RC::ConstHandle<Expr> const &shared
      )
    {
      return new CreateArrayTransform( location, input, operatorName, shared );
    }
    
    CreateArrayTransform::CreateArrayTransform(
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
    
    void CreateArrayTransform::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_input->appendJSON( jsonObjectEncoder.makeMember( "input" ), includeLocation );
      jsonObjectEncoder.makeMember( "operatorName" ).makeString( m_operatorName );
      m_shared->appendJSON( jsonObjectEncoder.makeMember( "shared" ), includeLocation );
    }
    
    void CreateArrayTransform::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_input->registerTypes( cgManager, diagnostics );
      if ( m_shared )
        m_shared->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType CreateArrayTransform::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      return m_input->getExprType( basicBlockBuilder );
    }
    
    CG::ExprValue CreateArrayTransform::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
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
        throw MR::ArrayTransformOperator::GetPrototypeException();

      CG::ExprType inputExprType = m_input->getExprType( basicBlockBuilder );
      if ( !RT::isArrayProducer( inputExprType.getAdapter()->getType() ) )
        throw CG::Error( getLocation(), "input must be an array producer" );
      RC::ConstHandle<CG::ArrayProducerAdapter> ioArrayProducerAdapter = RC::ConstHandle<CG::ArrayProducerAdapter>::StaticCast( inputExprType.getAdapter() );
      RC::ConstHandle<CG::Adapter> inputAdapter = ioArrayProducerAdapter->getElementAdapter();
      if ( operatorParams[0].getAdapter() != inputAdapter )
        throw CG::Error( getLocation(), "operator value parameter type (" + operatorParams[0].getAdapter()->getUserName() + ") does not match input array producer element type (" + inputAdapter->getUserName() + ")" );
      if ( operatorParams[0].getUsage() != CG::USAGE_LVALUE )
        throw CG::Error( getLocation(), "operator value parameter must be an 'io' parameter" );
      CG::ExprValue inputExprRValue = m_input->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );

      llvm::Value *resultLValue = ioArrayProducerAdapter->llvmAlloca( basicBlockBuilder, "result" );
      ioArrayProducerAdapter->llvmInit( basicBlockBuilder, resultLValue );
      basicBlockBuilder.getScope().put(
        CG::VariableSymbol::Create(
          CG::ExprValue(
            ioArrayProducerAdapter,
            CG::USAGE_LVALUE,
            context,
            resultLValue
            )
          )
        );
    
      bool needCall = true;
      if ( operatorParams.size() >= 2 )
      {
        if ( operatorParams[1].getAdapter() != sizeAdapter )
          throw CG::Error( getLocation(), "operator index parameter type (" + operatorParams[1].getAdapter()->getUserName() + ") must be 'Size'" );
        if ( operatorParams[1].getUsage() != CG::USAGE_RVALUE )
          throw CG::Error( getLocation(), "operator index parameter must be an 'in' parameter" );
          
        if ( operatorParams.size() >= 3 )
        {
          if ( operatorParams[2].getAdapter() != sizeAdapter )
            throw CG::Error( getLocation(), "operator index parameter type (" + operatorParams[2].getAdapter()->getUserName() + ") must be 'Size'" );
          if ( operatorParams[2].getUsage() != CG::USAGE_RVALUE )
            throw CG::Error( getLocation(), "operator index parameter must be an 'in' parameter" );
            
          if ( operatorParams.size() >= 4 )
          {
            if ( operatorParams.size() > 4 )
              throw MR::ArrayTransformOperator::GetPrototypeException();
              
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

            std::vector<llvm::Type const *> argTypes;
            argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) ); // function
            argTypes.push_back( sizeAdapter->llvmRType( context ) ); // numParams
            argTypes.push_back( ioArrayProducerAdapter->llvmLType( context ) ); // input array producer
            argTypes.push_back( sharedValueProducerAdapter->llvmLType( context ) ); // shared array producer
            argTypes.push_back( ioArrayProducerAdapter->llvmLType( context ) ); // output array producer
            llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
            llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateArrayTransform_4", funcType );
            
            std::vector<llvm::Value *> args;
            args.push_back( basicBlockBuilder->CreateBitCast(
              function->getLLVMFunction(),
              llvm::Type::getInt8PtrTy( llvmContext )
              ) );
            args.push_back( sizeAdapter->llvmConst( context, operatorParams.size() ) );
            args.push_back( inputExprRValue.getValue() );
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
        argTypes.push_back( ioArrayProducerAdapter->llvmLType( context ) ); // input array producer
        argTypes.push_back( ioArrayProducerAdapter->llvmLType( context ) ); // output array producer
        llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
        llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateArrayTransform_3", funcType );
        
        std::vector<llvm::Value *> args;
        args.push_back( basicBlockBuilder->CreateBitCast(
          function->getLLVMFunction(),
          llvm::Type::getInt8PtrTy( llvmContext )
          ) );
        args.push_back( sizeAdapter->llvmConst( context, operatorParams.size() ) );
        args.push_back( inputExprRValue.getValue() );
        args.push_back( resultLValue );
        basicBlockBuilder->CreateCall( func, args.begin(), args.end() );
      }

      return CG::ExprValue(
        ioArrayProducerAdapter,
        CG::USAGE_RVALUE,
        context,
        ioArrayProducerAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue )
        );
    }
  }
}
