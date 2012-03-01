/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ArrayProducerAdapter.h"
#include "BooleanAdapter.h"
#include "ByteAdapter.h"
#include "IntegerAdapter.h"
#include "SizeAdapter.h"
#include "StringAdapter.h"
#include "ConstStringAdapter.h"
#include "FloatAdapter.h"
#include "OpaqueAdapter.h"
#include "VariableArrayAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "BasicBlockBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/RT/ArrayProducerDesc.h>
#include <Fabric/Core/RT/StringImpl.h>
#include <Fabric/Core/RT/VariableArrayDesc.h>
#include <Fabric/Core/MR/ArrayProducer.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Intrinsics.h>
#include <llvm/GlobalVariable.h>

namespace Fabric
{
  namespace CG
  {
    ArrayProducerAdapter::ArrayProducerAdapter(
      RC::ConstHandle<Manager> const &manager,
      RC::ConstHandle<RT::ArrayProducerDesc> const &arrayProducerDesc
      )
      : Adapter( manager, arrayProducerDesc, FL_PASS_BY_REFERENCE )
      , m_arrayProducerDesc( arrayProducerDesc )
      , m_elementAdapter( manager->getAdapter( arrayProducerDesc->getElementDesc() ) )
      , m_elementVariableArrayAdapter( manager->getVariableArrayOf( m_elementAdapter ) )
    {
    }
    
    llvm::Type const *ArrayProducerAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
      return llvm::Type::getInt8Ty( llvmContext )->getPointerTo()->getPointerTo();
    }
    
    void ArrayProducerAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
        
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      m_elementAdapter->llvmCompileToModule( moduleBuilder );
      m_elementVariableArrayAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
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
          llvm::Value *arrayProducerRValue = basicBlockBuilder->CreateLoad( rValue );
          basicBlockBuilder->CreateStore(
            basicBlockBuilder->CreateIsNotNull( arrayProducerRValue ),
            booleanLValue
            );
          basicBlockBuilder->CreateRetVoid();
        }
      }

      {
        MethodBuilder functionBuilder( moduleBuilder, sizeAdapter, this, USAGE_RVALUE, "getCount" );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *rValue = functionBuilder[0];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          basicBlockBuilder->CreateRet(
            llvmGetCount( basicBlockBuilder, rValue )
            );
        }
      }

      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          m_elementVariableArrayAdapter,
          this, USAGE_RVALUE,
          "produce"
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *rValue = functionBuilder[0];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvmProduce0( basicBlockBuilder, rValue, functionBuilder.getScope().llvmGetReturnLValue() );
          basicBlockBuilder->CreateRetVoid();
        }
      }

      {
        MethodBuilder functionBuilder(
          moduleBuilder, m_elementAdapter,
          this, USAGE_RVALUE,
          "produce",
          "index", sizeAdapter, USAGE_RVALUE
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *rValue = functionBuilder[0];
          llvm::Value *indexRValue = functionBuilder[1];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          CG::FunctionScope &functionScope = functionBuilder.getScope();
          functionScope.llvmPrepareReturnLValue( basicBlockBuilder );
          llvmProduce1( basicBlockBuilder, rValue, indexRValue, functionScope.llvmGetReturnLValue() );
          if ( functionScope.getReturnInfo().usesReturnLValue() )
            basicBlockBuilder->CreateRetVoid();
          else
            basicBlockBuilder->CreateRet( basicBlockBuilder->CreateLoad( functionScope.llvmGetReturnLValue() ) );
        }
      }

      {
        MethodBuilder functionBuilder(
          moduleBuilder, m_elementVariableArrayAdapter,
          this, USAGE_RVALUE,
          "produce",
          "index", sizeAdapter, USAGE_RVALUE,
          "count", sizeAdapter, USAGE_RVALUE
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *rValue = functionBuilder[0];
          llvm::Value *indexRValue = functionBuilder[1];
          llvm::Value *countRValue = functionBuilder[2];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvmProduce2( basicBlockBuilder, rValue, indexRValue, countRValue, functionBuilder.getScope().llvmGetReturnLValue() );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        MethodBuilder functionBuilder( moduleBuilder, 0, this, USAGE_RVALUE, "flush" );
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
    
    void ArrayProducerAdapter::Retain( void const *rValue )
    {
      MR::ArrayProducer const *arrayProducer = *static_cast<MR::ArrayProducer const * const *>( rValue );
      if ( arrayProducer )
        arrayProducer->retain();
    }
    
    void ArrayProducerAdapter::Release( void const *rValue )
    {
      MR::ArrayProducer const *arrayProducer = *static_cast<MR::ArrayProducer const * const *>( rValue );
      if ( arrayProducer )
        arrayProducer->release();
    }
    
    void ArrayProducerAdapter::DefaultAssign( void const *srcRValue, void *dstLValue )
    {
      MR::ArrayProducer const *srcValueProducer = *static_cast<MR::ArrayProducer const * const *>( srcRValue );
      MR::ArrayProducer const *&dstValueProducer = *static_cast<MR::ArrayProducer const **>( dstLValue );
      
      if ( srcValueProducer != dstValueProducer )
      {
        if ( dstValueProducer )
          dstValueProducer->release();
        
        dstValueProducer = srcValueProducer;
        
        if ( dstValueProducer )
          dstValueProducer->retain();
      }
    }
    
    size_t ArrayProducerAdapter::GetCount( void const *arrayProducerRValue )
    {
      MR::ArrayProducer *arrayProducer = *static_cast<MR::ArrayProducer * const *>( arrayProducerRValue );
      if ( arrayProducer )
        return arrayProducer->createComputeState()->getCount();
      else return 0;
    }

    void ArrayProducerAdapter::Flush( void const *arrayProducerRValue )
    {
      MR::ArrayProducer *arrayProducer = *static_cast<MR::ArrayProducer * const *>( arrayProducerRValue );
      if ( arrayProducer )
        arrayProducer->flush();
    }
    
    void ArrayProducerAdapter::Produce0( void const *_adapter, void const *arrayProducerRValue, void *dstLValue )
    {
      MR::ArrayProducer *arrayProducer = *static_cast<MR::ArrayProducer * const *>( arrayProducerRValue );
      if ( arrayProducer )
      {
        RC::Handle<MR::ArrayProducer::ComputeState> computeState = arrayProducer->createComputeState();
        size_t count = computeState->getCount();
        RC::ConstHandle<RT::VariableArrayDesc> elementVariableArrayDesc = RC::ConstHandle<RT::VariableArrayDesc>::StaticCast( static_cast<ArrayProducerAdapter const *>( _adapter )->m_elementVariableArrayAdapter->getDesc() );
        elementVariableArrayDesc->setNumMembers( dstLValue, count );
        void *firstMemberData = elementVariableArrayDesc->getMutableMemberData( dstLValue, 0 );
        computeState->produce( 0, count, firstMemberData );
      }
    }
    
    void ArrayProducerAdapter::Produce1( void const *arrayProducerRValue, size_t indexRValue, void *dstLValue )
    {
      MR::ArrayProducer *arrayProducer = *static_cast<MR::ArrayProducer * const *>( arrayProducerRValue );
      if ( arrayProducer )
        arrayProducer->createComputeState()->produce( indexRValue, dstLValue );
    }
    
    void ArrayProducerAdapter::Produce2( void const *_adapter, void const *arrayProducerRValue, size_t indexRValue, size_t countRValue, void *dstLValue )
    {
      MR::ArrayProducer *arrayProducer = *static_cast<MR::ArrayProducer * const *>( arrayProducerRValue );
      if ( arrayProducer )
      {
        RC::ConstHandle<RT::VariableArrayDesc> elementVariableArrayDesc =  RC::ConstHandle<RT::VariableArrayDesc>::StaticCast( static_cast<ArrayProducerAdapter const *>( _adapter )->m_elementVariableArrayAdapter->getDesc() );
        elementVariableArrayDesc->setNumMembers( dstLValue, countRValue );
        void *firstMemberData = elementVariableArrayDesc->getMutableMemberData( dstLValue, 0 );
        arrayProducer->createComputeState()->produce( indexRValue, countRValue, firstMemberData );
      }
    }
    
    void *ArrayProducerAdapter::llvmResolveExternalFunction( std::string const &functionName ) const
    {
      if ( functionName == "__"+getCodeName()+"__Retain" )
        return (void *)&ArrayProducerAdapter::Retain;
      else if ( functionName == "__"+getCodeName()+"__Release" )
        return (void *)&ArrayProducerAdapter::Release;
      else if ( functionName == "__"+getCodeName()+"__DefaultAssign" )
        return (void *)&ArrayProducerAdapter::DefaultAssign;
      else if ( functionName == "__"+getCodeName()+"__GetCount" )
        return (void *)&ArrayProducerAdapter::GetCount;
      else if ( functionName == "__"+getCodeName()+"__Produce0" )
        return (void *)&ArrayProducerAdapter::Produce0;
      else if ( functionName == "__"+getCodeName()+"__Produce1" )
        return (void *)&ArrayProducerAdapter::Produce1;
      else if ( functionName == "__"+getCodeName()+"__Produce2" )
        return (void *)&ArrayProducerAdapter::Produce2;
      else if ( functionName == "__"+getCodeName()+"__Flush" )
        return (void *)&ArrayProducerAdapter::Flush;
      else return Adapter::llvmResolveExternalFunction( functionName );
    }

    void ArrayProducerAdapter::llvmInit( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      llvm::PointerType const *rawType = static_cast<llvm::PointerType const *>( llvmRawType( basicBlockBuilder.getContext() ) );
      basicBlockBuilder->CreateStore(
        llvm::ConstantPointerNull::get( rawType ),
        lValue
        );
    }
    
    void ArrayProducerAdapter::llvmRetain( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Retain", funcType ); 
      basicBlockBuilder->CreateCall( func, rValue );
    }
    
    void ArrayProducerAdapter::llvmRelease( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Release", funcType ); 
      basicBlockBuilder->CreateCall( func, rValue );
    }
    
    llvm::Value *ArrayProducerAdapter::llvmGetCount(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *arrayProducerRValue
      ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvmSizeType( context ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__GetCount", funcType ); 
      return basicBlockBuilder->CreateCall( func, arrayProducerRValue );
    }
    
    void ArrayProducerAdapter::llvmProduce0(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *arrayProducerRValue,
      llvm::Value *dstLValue
      ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
      argTypes.push_back( llvmRType( context ) );
      argTypes.push_back( m_elementVariableArrayAdapter->llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Produce0", funcType ); 
      basicBlockBuilder->CreateCall3( func, llvmAdapterPtr( basicBlockBuilder ), arrayProducerRValue, dstLValue );
    }
    
    void ArrayProducerAdapter::llvmProduce1(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *arrayProducerRValue,
      llvm::Value *indexRValue,
      llvm::Value *dstLValue
      ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      argTypes.push_back( llvmSizeType( context ) );
      argTypes.push_back( m_elementAdapter->llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Produce1", funcType ); 
      basicBlockBuilder->CreateCall3( func, arrayProducerRValue, indexRValue, dstLValue );
    }
    
    void ArrayProducerAdapter::llvmProduce2(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *arrayProducerRValue,
      llvm::Value *indexRValue,
      llvm::Value *countRValue,
      llvm::Value *dstLValue
      ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
      argTypes.push_back( llvmRType( context ) );
      argTypes.push_back( llvmSizeType( context ) );
      argTypes.push_back( llvmSizeType( context ) );
      argTypes.push_back( m_elementVariableArrayAdapter->llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Produce2", funcType ); 
      basicBlockBuilder->CreateCall5( func, llvmAdapterPtr( basicBlockBuilder ), arrayProducerRValue, indexRValue, countRValue, dstLValue );
    }

    void ArrayProducerAdapter::llvmFlush(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *arrayProducerRValue
      ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Flush", funcType ); 
      basicBlockBuilder->CreateCall( func, arrayProducerRValue );
    }
 
    void ArrayProducerAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__DefaultAssign", funcType ); 
      basicBlockBuilder->CreateCall2( func, srcRValue, dstLValue );
    }

    void ArrayProducerAdapter::llvmDisposeImpl( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      llvmRelease( basicBlockBuilder, lValue );
    }
    
    std::string ArrayProducerAdapter::toString( void const *data ) const
    {
      return m_arrayProducerDesc->getUserName();
    }
    
    llvm::Constant *ArrayProducerAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvm::ConstantPointerNull::get( static_cast<llvm::PointerType const *>( llvmRawType( basicBlockBuilder.getContext() ) ) );
    }
      
    llvm::Constant *ArrayProducerAdapter::llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmDefaultLValue( basicBlockBuilder );
    }

    llvm::Constant *ArrayProducerAdapter::llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const
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
    
    RC::ConstHandle<Adapter> ArrayProducerAdapter::getElementAdapter() const
    {
      return m_elementAdapter;
    }
  }
}
