/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "ValueProducerAdapter.h"
#include "BooleanAdapter.h"
#include "ByteAdapter.h"
#include "IntegerAdapter.h"
#include "SizeAdapter.h"
#include "StringAdapter.h"
#include "ConstStringAdapter.h"
#include "FloatAdapter.h"
#include "OpaqueAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "BasicBlockBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/RT/ValueProducerDesc.h>
#include <Fabric/Core/RT/StringImpl.h>
#include <Fabric/Core/MR/ValueProducer.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Intrinsics.h>
#include <llvm/GlobalVariable.h>

namespace Fabric
{
  namespace CG
  {
    ValueProducerAdapter::ValueProducerAdapter(
      RC::ConstHandle<Manager> const &manager,
      RC::ConstHandle<RT::ValueProducerDesc> const &valueProducerDesc
      )
      : Adapter( manager, valueProducerDesc, FL_PASS_BY_REFERENCE )
      , m_valueProducerDesc( valueProducerDesc )
      , m_valueAdapter( manager->getAdapter( valueProducerDesc->getValueDesc() ) )
    {
    }
    
    llvm::Type const *ValueProducerAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
      return llvm::Type::getInt8Ty( llvmContext )->getPointerTo()->getPointerTo();
    }
    
    void ValueProducerAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
        
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      m_valueAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<ConstStringAdapter> constStringAdapter = getManager()->getConstStringAdapter();
      constStringAdapter->llvmCompileToModule( moduleBuilder );
      
      moduleBuilder->addTypeName( getCodeName(), llvmRawType( context ) );
      
      static const bool buildFunctions = true;
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, stringAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          std::string name = getUserName();
          ExprValue exprValue( constStringAdapter, USAGE_RVALUE, context, constStringAdapter->llvmConst( basicBlockBuilder, name.data(), name.length() ) );
          llvm::Value *stringRValue = stringAdapter->llvmCast( basicBlockBuilder, exprValue );
          stringAdapter->llvmAssign( basicBlockBuilder, stringLValue, stringRValue );
          stringAdapter->llvmDispose( basicBlockBuilder, stringRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
   
      {
        ConstructorBuilder functionBuilder( moduleBuilder, booleanAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *booleanLValue = functionBuilder[0];
          llvm::Value *rValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *valueProducerRValue = basicBlockBuilder->CreateLoad( rValue );
          basicBlockBuilder->CreateStore(
            basicBlockBuilder->CreateIsNotNull( valueProducerRValue ),
            booleanLValue
            );
          basicBlockBuilder->CreateRetVoid();
        }
      }

      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          m_valueAdapter,
          this, USAGE_RVALUE,
          "produce"
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *rValue = functionBuilder[0];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          CG::FunctionScope &functionScope = functionBuilder.getScope();
          functionScope.llvmPrepareReturnLValue( basicBlockBuilder );
          llvmProduce( basicBlockBuilder, rValue, functionScope.llvmGetReturnLValue() );
          if ( functionScope.getReturnInfo().usesReturnLValue() )
            basicBlockBuilder->CreateRetVoid();
          else
            basicBlockBuilder->CreateRet( basicBlockBuilder->CreateLoad( functionScope.llvmGetReturnLValue() ) );
        }
      }
 
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          0,
          this, USAGE_RVALUE,
          "flush"
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *rValue = functionBuilder[0];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );

          llvmFlush( basicBlockBuilder, rValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
    }
    
    void ValueProducerAdapter::Retain( void const *rValue )
    {
      MR::ValueProducer const *valueProducer = *static_cast<MR::ValueProducer const * const *>( rValue );
      if ( valueProducer )
        valueProducer->retain();
    }
    
    void ValueProducerAdapter::Release( void const *rValue )
    {
      MR::ValueProducer const *valueProducer = *static_cast<MR::ValueProducer const * const *>( rValue );
      if ( valueProducer )
        valueProducer->release();
    }
    
    void ValueProducerAdapter::DefaultAssign( void const *srcRValue, void *dstLValue )
    {
      MR::ValueProducer const *srcValueProducer = *static_cast<MR::ValueProducer const * const *>( srcRValue );
      MR::ValueProducer const *&dstValueProducer = *static_cast<MR::ValueProducer const **>( dstLValue );
      
      if ( srcValueProducer != dstValueProducer )
      {
        if ( dstValueProducer )
          dstValueProducer->release();
        
        dstValueProducer = srcValueProducer;
        
        if ( dstValueProducer )
          dstValueProducer->retain();
      }
    }
    
    void ValueProducerAdapter::Flush( void const *valueProducerRValue )
    {
      MR::ValueProducer *valueProducer = *static_cast<MR::ValueProducer * const *>( valueProducerRValue );
      if ( valueProducer )
        valueProducer->flush();
    }
    
    void ValueProducerAdapter::Produce( void const *valueProducerRValue, void *dstLValue )
    {
      MR::ValueProducer *valueProducer = *static_cast<MR::ValueProducer * const *>( valueProducerRValue );
      if ( valueProducer )
        valueProducer->createComputeState()->produce( dstLValue );
    }
    
    void *ValueProducerAdapter::llvmResolveExternalFunction( std::string const &functionName ) const
    {
      if ( functionName == "__"+getCodeName()+"__Retain" )
        return (void *)&ValueProducerAdapter::Retain;
      else if ( functionName == "__"+getCodeName()+"__Release" )
        return (void *)&ValueProducerAdapter::Release;
      else if ( functionName == "__"+getCodeName()+"__DefaultAssign" )
        return (void *)&ValueProducerAdapter::DefaultAssign;
      else if ( functionName == "__"+getCodeName()+"__Produce" )
        return (void *)&ValueProducerAdapter::Produce;
      else if ( functionName == "__"+getCodeName()+"__Flush" )
        return (void *)&ValueProducerAdapter::Flush;
      else return Adapter::llvmResolveExternalFunction( functionName );
    }

    void ValueProducerAdapter::llvmInit( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      llvm::PointerType const *rawType = static_cast<llvm::PointerType const *>( llvmRawType( basicBlockBuilder.getContext() ) );
      basicBlockBuilder->CreateStore(
        llvm::ConstantPointerNull::get( rawType ),
        lValue
        );
    }
    
    void ValueProducerAdapter::llvmRetain( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Retain", funcType ); 
      basicBlockBuilder->CreateCall( func, rValue );
    }
    
    void ValueProducerAdapter::llvmRelease( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Release", funcType ); 
      basicBlockBuilder->CreateCall( func, rValue );
    }
    
    void ValueProducerAdapter::llvmFlush( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *valueProducerRValue ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Flush", funcType ); 
      basicBlockBuilder->CreateCall( func, valueProducerRValue );
    }

    void ValueProducerAdapter::llvmProduce( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *valueProducerRValue, llvm::Value *dstLValue ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      argTypes.push_back( m_valueAdapter->llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Produce", funcType ); 
      basicBlockBuilder->CreateCall2( func, valueProducerRValue, dstLValue );
    }

    void ValueProducerAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__DefaultAssign", funcType ); 
      basicBlockBuilder->CreateCall2( func, srcRValue, dstLValue );
    }

    void ValueProducerAdapter::llvmDisposeImpl( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      llvmRelease( basicBlockBuilder, lValue );
    }
    
    std::string ValueProducerAdapter::toString( void const *data ) const
    {
      return m_valueProducerDesc->getUserName();
    }
    
    llvm::Constant *ValueProducerAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvm::ConstantPointerNull::get( static_cast<llvm::PointerType const *>( llvmRawType( basicBlockBuilder.getContext() ) ) );
    }
      
    llvm::Constant *ValueProducerAdapter::llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmDefaultLValue( basicBlockBuilder );
    }

    llvm::Constant *ValueProducerAdapter::llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      llvm::Constant *defaultValue = llvmDefaultValue( basicBlockBuilder );
      return new llvm::GlobalVariable(
        *basicBlockBuilder.getModuleBuilder(),
        defaultValue->getType(),
        true,
        llvm::GlobalValue::InternalLinkage,
        defaultValue,
        "__" + getCodeName() + "__DefaultValue"
        );
    }
    
    RC::ConstHandle<Adapter> ValueProducerAdapter::getValueAdapter() const
    {
      return m_valueAdapter;
    }
  }
}
