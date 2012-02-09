/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "ContainerAdapter.h"
#include "BooleanAdapter.h"
#include "SizeAdapter.h"
#include "StringAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "FunctionBuilder.h"
#include "BasicBlockBuilder.h"
#include "OverloadNames.h"

#include <Fabric/Core/RT/ContainerDesc.h>
#include <Fabric/Core/RT/ContainerImpl.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Intrinsics.h>
#include <llvm/GlobalVariable.h>

namespace Fabric
{
  namespace CG
  {
    ContainerAdapter::ContainerAdapter(
      RC::ConstHandle<Manager> const &manager,
      RC::ConstHandle<RT::ContainerDesc> const &containerDesc
      )
      : Adapter( manager, containerDesc, FL_PASS_BY_REFERENCE )
      , m_containerDesc( containerDesc )
    {
    }

    llvm::Type const *ContainerAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
      return llvm::Type::getInt8Ty( llvmContext )->getPointerTo()->getPointerTo();
    }
    
    void ContainerAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
        
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
      
      moduleBuilder->addTypeName( getCodeName(), llvmRawType( context ) );
      
      static const bool buildFunctions = true;
      
      {
        std::string name = constructOverloadName( stringAdapter, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "stringLValue", stringAdapter, USAGE_LVALUE ) );
        params.push_back( FunctionParam( "arrayRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType(), params );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *arrayRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *arrayLValue = llvmRValueToLValue( basicBlockBuilder, arrayRValue );
          stringAdapter->llvmCallCast( basicBlockBuilder, this, arrayLValue, stringLValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
   
      {
        std::string name = constructOverloadName( booleanAdapter, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "booleanLValue", booleanAdapter, USAGE_LVALUE ) );
        params.push_back( FunctionParam( "rValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType(), params );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          llvm::Value *rValue = functionBuilder[0];
          basicBlockBuilder->SetInsertPoint( basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" ) );
          basicBlockBuilder->CreateRet( llvmToBoolean( basicBlockBuilder, rValue ) );
        }
      }

      {
        std::string name = methodOverloadName( "setCount", this );
        std::vector<FunctionParam> params;
        params.push_back( FunctionParam( "selfLValue", this, CG::USAGE_LVALUE ) );
        params.push_back( FunctionParam( "newSizeRValue", sizeAdapter, CG::USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType(), params, false, 0, true );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          llvm::Value *selfLValue = functionBuilder[0];
          llvm::Value *newSizeRValue = functionBuilder[1];
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvmSetCount( basicBlockBuilder, selfLValue, newSizeRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
 
      {
        std::string name = methodOverloadName( "getCount", this );
        std::vector<FunctionParam> params;
        params.push_back( FunctionParam( "rValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( sizeAdapter, USAGE_RVALUE ), params );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          llvm::Value *rValue = functionBuilder[0];
          basicBlockBuilder->SetInsertPoint( basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" ) );
          basicBlockBuilder->CreateRet( llvmGetCount( basicBlockBuilder, rValue ) );
        }
      }
    }

    void *ContainerAdapter::llvmResolveExternalFunction( std::string const &functionName ) const
    {
      if ( functionName == "__"+getCodeName()+"__DefaultAssign" )
        return (void *)&RT::ContainerImpl::SetData;
      else if ( functionName == "__"+getCodeName()+"__Dispose" )
        return (void *)&RT::ContainerImpl::DisposeData;
      else if ( functionName == "__"+getCodeName()+"__ToBoolean" )
        return (void *)&RT::ContainerImpl::IsValid;
      else if ( functionName == "__"+getCodeName()+"__GetCount" )
        return (void *)&RT::ContainerImpl::GetCount;
      else if ( functionName == "__"+getCodeName()+"__SetCount" )
        return (void *)&RT::ContainerImpl::SetCount;
      else return Adapter::llvmResolveExternalFunction( functionName );
    }

    void ContainerAdapter::llvmInit( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      llvm::PointerType const *rawType = static_cast<llvm::PointerType const *>( llvmRawType( basicBlockBuilder.getContext() ) );
      basicBlockBuilder->CreateStore(
        llvm::ConstantPointerNull::get( rawType ),
        lValue
        );
    }

    void ContainerAdapter::llvmSetCount(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *containerLValue,
      llvm::Value *newSizeRValue
      ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmLType( context ) );
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__SetCount", funcType ); 
      std::vector< llvm::Value * > args;
      args.push_back( containerLValue );
      args.push_back( newSizeRValue );
      basicBlockBuilder->CreateCall( func, args.begin(), args.end() );
    }

    llvm::Value *ContainerAdapter::llvmGetCount(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *containerRValue
      ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvmSizeType( context ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__GetCount", funcType ); 
      return basicBlockBuilder->CreateCall( func, containerRValue );
    }

    llvm::Value *ContainerAdapter::llvmToBoolean(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *containerRValue
      ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvmSizeType( context ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__ToBoolean", funcType ); 
      return basicBlockBuilder->CreateCall( func, containerRValue );
    }

    void ContainerAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__DefaultAssign", funcType ); 
      basicBlockBuilder->CreateCall2( func, srcRValue, dstLValue );
    }

    void ContainerAdapter::llvmDisposeImpl( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Dispose", funcType ); 
      basicBlockBuilder->CreateCall( func, lValue );
    }
    
    std::string ContainerAdapter::toString( void const *data ) const
    {
      return RT::ContainerImpl::GetName( data );
    }

    llvm::Constant *ContainerAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvm::ConstantPointerNull::get( static_cast<llvm::PointerType const *>( llvmRawType( basicBlockBuilder.getContext() ) ) );
    }
      
    llvm::Constant *ContainerAdapter::llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmDefaultLValue( basicBlockBuilder );
    }

    llvm::Constant *ContainerAdapter::llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const
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
  }
}
