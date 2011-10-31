/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "ConstStringAdapter.h"
#include "BooleanAdapter.h"
#include "IntegerAdapter.h"
#include "SizeAdapter.h"
#include "StringAdapter.h"
#include "OpaqueAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "FunctionBuilder.h"
#include "BasicBlockBuilder.h"
#include "OverloadNames.h"

#include <Fabric/Core/RT/ConstStringDesc.h>

#include <llvm/Module.h>
#include <llvm/Function.h>

namespace Fabric
{
  namespace CG
  {
    ConstStringAdapter::ConstStringAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::ConstStringDesc> const &constStringDesc )
      : Adapter( manager, constStringDesc, FL_PASS_BY_REFERENCE )
      , m_constStringDesc( constStringDesc )
    {
    }
    
    llvm::Type const *ConstStringAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      std::vector<llvm::Type const *> memberLLVMTypes;
      memberLLVMTypes.push_back( llvm::Type::getInt8PtrTy( context->getLLVMContext() ) );
      memberLLVMTypes.push_back( llvmSizeType( context ) );
      return llvm::StructType::get( context->getLLVMContext(), memberLLVMTypes, true );
    }
    
    void ConstStringAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
      
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );
      
      moduleBuilder->addTypeName( getCodeName(), llvmRawType( context ) );
      
      static const bool buildFunctions = true;

      {
        // [pzion 20110202] Cast string to boolean
        std::string name = constructOverloadName( booleanAdapter, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "booleanLValue", booleanAdapter, USAGE_LVALUE ) );
        params.push_back( FunctionParam( "constStringRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType(), params );
        if ( buildFunctions )
        {
          llvm::Value *booleanLValue = functionBuilder[0];
          llvm::Value *constStringRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *lengthLValue = basicBlockBuilder->CreateStructGEP( constStringRValue, 1 );
          llvm::Value *lengthRValue = basicBlockBuilder->CreateLoad( lengthLValue );
          llvm::Value *booleanRValue = basicBlockBuilder->CreateICmpNE( lengthRValue, llvm::ConstantInt::get( llvmSizeType( context ), 0, false ) );
          booleanAdapter->llvmDefaultAssign( basicBlockBuilder, booleanLValue, booleanRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        std::string name = constructOverloadName( stringAdapter, this );
        std::vector<FunctionParam> params;
        params.push_back( FunctionParam( "stringLValue", stringAdapter, USAGE_LVALUE ) );
        params.push_back( FunctionParam( "constStringRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType(), params );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *constStringRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *constStringLValue = llvmRValueToLValue( basicBlockBuilder, constStringRValue );
          stringAdapter->llvmCallCast( basicBlockBuilder, this, constStringLValue, stringLValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
    }
    
    llvm::Constant *ConstStringAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmConst( basicBlockBuilder, m_constStringDesc->getValueData( m_constStringDesc->getDefaultData() ), m_constStringDesc->getValueLength( m_constStringDesc->getDefaultData() ) );
    }
      
    llvm::Constant *ConstStringAdapter::llvmConst( CG::BasicBlockBuilder &basicBlockBuilder, char const *data, size_t length ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      
      llvm::Type const *charType = llvm::Type::getInt8Ty( context->getLLVMContext() );
      std::vector<llvm::Constant *> chars;
      for ( size_t i=0; i<length; ++i )
        chars.push_back( llvm::ConstantInt::get( charType, data[i] ) );
      llvm::Constant *dataConstant = llvm::ConstantArray::get( llvm::ArrayType::get( charType, length ), chars );

      llvm::Value *dataGlobal = new llvm::GlobalVariable(
        *basicBlockBuilder.getModuleBuilder(),
        dataConstant->getType(),
        true,
        llvm::GlobalValue::InternalLinkage,
        dataConstant,
        ""
        );

      llvm::Constant *llvmData = llvm::cast<llvm::Constant>( basicBlockBuilder->CreateConstGEP2_32( dataGlobal, 0, 0 ) );
      llvm::Constant *llvmLength = llvm::ConstantInt::get( llvmSizeType( context ), length, false );
      
      std::vector<llvm::Constant *> elementDefaultRValues;
      elementDefaultRValues.push_back( llvmData );
      elementDefaultRValues.push_back( llvmLength );
      llvm::Constant *llvmStruct = llvm::ConstantStruct::get( (llvm::StructType const *)llvmRawType( context ), elementDefaultRValues );

      return new llvm::GlobalVariable(
        *basicBlockBuilder.getModuleBuilder(),
        llvmStruct->getType(),
        true,
        llvm::GlobalValue::InternalLinkage,
        llvmStruct,
        ""
        );
    }
    
    std::string ConstStringAdapter::toString( void const *data ) const
    {
      return m_constStringDesc->toString( data );
    }

    void ConstStringAdapter::llvmRetain( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {
    }

    void ConstStringAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      basicBlockBuilder->CreateStore( srcRValue, dstLValue );
    }
    
    void ConstStringAdapter::llvmRelease( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {
    }
  };
};
