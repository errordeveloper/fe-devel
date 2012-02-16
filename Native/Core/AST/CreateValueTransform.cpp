/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/AST/CreateValueTransform.h>
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
    FABRIC_AST_NODE_IMPL( CreateValueTransform );
    
    RC::ConstHandle<CreateValueTransform> CreateValueTransform::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &input,
      std::string const &operatorName,
      RC::ConstHandle<Expr> const &shared
      )
    {
      return new CreateValueTransform( location, input, operatorName, shared );
    }
    
    CreateValueTransform::CreateValueTransform(
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
    
    void CreateValueTransform::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_input->appendJSON( jsonObjectEncoder.makeMember( "input" ), includeLocation );
      jsonObjectEncoder.makeMember( "operatorName" ).makeString( m_operatorName );
      m_shared->appendJSON( jsonObjectEncoder.makeMember( "shared" ), includeLocation );
    }
    
    void CreateValueTransform::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_input->registerTypes( cgManager, diagnostics );
      if ( m_shared )
        m_shared->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType CreateValueTransform::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      return m_input->getExprType( basicBlockBuilder );
    }
    
    CG::ExprValue CreateValueTransform::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "cannot be used as l-values" );
      
      RC::Handle<CG::Context> context = basicBlockBuilder.getContext();
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
      
      CG::ExprType inputExprType = m_input->getExprType( basicBlockBuilder );
      if ( !RT::isValueProducer( inputExprType.getAdapter()->getType() ) )
        throw CG::Error( getLocation(), "input must be a value producer" );
      RC::ConstHandle<CG::ValueProducerAdapter> valueProducerAdapter = RC::ConstHandle<CG::ValueProducerAdapter>::StaticCast( inputExprType.getAdapter() );
      RC::ConstHandle<CG::Adapter> valueAdapter = valueProducerAdapter->getValueAdapter();
      
      RC::ConstHandle<CG::Symbol> operatorSymbol = basicBlockBuilder.getScope().get( m_operatorName );
      if ( !operatorSymbol )
        throw CG::Error( getLocation(), _(m_operatorName) + ": operator not found" );
      if ( !operatorSymbol->isPencil() )
        throw CG::Error( getLocation(), _(m_operatorName) + ": not an operator" );
      RC::ConstHandle<CG::PencilSymbol> pencil = RC::ConstHandle<CG::PencilSymbol>::StaticCast( operatorSymbol );
      CG::Function const *function = pencil->getUniqueFunction( getLocation(), "operator " + _(m_operatorName) );
      std::vector<CG::FunctionParam> const &operatorParams = function->getParams();
      CG::ExprValue inputExprRValue = m_input->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
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

      if ( !m_shared )
      {
        if ( operatorParams.size() != 1
          || operatorParams[0].getAdapter() != valueAdapter
          || operatorParams[0].getUsage() != CG::USAGE_LVALUE
          )
          throw CG::Error( getLocation(), "operator must take parameters: io " + valueAdapter->getUserName() + " value" );
      
        std::vector<llvm::Type const *> argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( valueProducerAdapter->llvmLType( context ) );
        argTypes.push_back( valueProducerAdapter->llvmLType( context ) );
        llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
        llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateValueTransform_1", funcType );
        
        basicBlockBuilder->CreateCall3(
          func,
          basicBlockBuilder->CreateBitCast(
            function->getLLVMFunction(),
            llvm::Type::getInt8PtrTy( llvmContext )
            ),
          inputExprRValue.getValue(),
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
          || operatorParams[0].getAdapter() != valueAdapter
          || operatorParams[0].getUsage() != CG::USAGE_LVALUE
          || operatorParams[1].getAdapter() != sharedAdapter
          || operatorParams[1].getUsage() != CG::USAGE_RVALUE
          )
          throw CG::Error( getLocation(), "operator must take parameters: io " + valueAdapter->getUserName() + " value, in " + sharedAdapter->getUserName() + " shared" );
      
        std::vector<llvm::Type const *> argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( llvmContext ) );
        argTypes.push_back( valueProducerAdapter->llvmLType( context ) );
        argTypes.push_back( sharedValueProducerAdapter->llvmLType( context ) );
        argTypes.push_back( valueProducerAdapter->llvmLType( context ) );
        llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( llvmContext ), argTypes, false );
        llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__MR_CreateValueTransform_2", funcType );
        
        basicBlockBuilder->CreateCall4(
          func,
          basicBlockBuilder->CreateBitCast(
            function->getLLVMFunction(),
            llvm::Type::getInt8PtrTy( llvmContext )
            ),
          inputExprRValue.getValue(),
          sharedExprRValue.getValue(),
          resultLValue
          );
      }
      
      return CG::ExprValue(
        valueProducerAdapter,
        CG::USAGE_RVALUE,
        context,
        valueProducerAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue )
        );
    }
  }
}
