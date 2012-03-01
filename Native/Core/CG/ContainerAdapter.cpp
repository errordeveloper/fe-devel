/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ContainerAdapter.h"
#include "BooleanAdapter.h"
#include "SizeAdapter.h"
#include "StringAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "FunctionBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "BasicBlockBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

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
      
      llvm::Function *toStringFunction = 0;
      {
        ConstructorBuilder functionBuilder( moduleBuilder, stringAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *containerRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *containerLValue = llvmRValueToLValue( basicBlockBuilder, containerRValue );
          stringAdapter->llvmCallCast( basicBlockBuilder, this, containerLValue, stringLValue );
          basicBlockBuilder->CreateRetVoid();
          toStringFunction = functionBuilder.getLLVMFunction();
        }
      }

      {
        ConstructorBuilder functionBuilder( moduleBuilder, booleanAdapter, this );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          llvm::Value *booleanLValue = functionBuilder[0];
          llvm::Value *containerRValue = functionBuilder[1];
          basicBlockBuilder->SetInsertPoint( basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" ) );
          
          std::vector<llvm::Type const *> argTypes;
          argTypes.push_back( llvmRType( context ) );
          llvm::FunctionType const *funcType = llvm::FunctionType::get( booleanAdapter->llvmRType( context ), argTypes, false );
          llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__ToBoolean", funcType ); 
          llvm::Value *booleanRValue = basicBlockBuilder->CreateCall( func, containerRValue );
          booleanAdapter->llvmAssign( basicBlockBuilder, booleanLValue, booleanRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }

      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          0,
          this, USAGE_LVALUE,
          "resize",
          "newSize", sizeAdapter, USAGE_RVALUE
          );
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
        MethodBuilder functionBuilder(
          moduleBuilder,
          sizeAdapter,
          this, USAGE_RVALUE,
          "size"
          );
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
        return (void *)&RT::ContainerImpl::size;
      else if ( functionName == "__"+getCodeName()+"__SetCount" )
        return (void *)&RT::ContainerImpl::resize;
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
      argTypes.push_back( llvmSizeType( context ) );
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
