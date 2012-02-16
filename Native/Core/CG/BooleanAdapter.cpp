/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "BooleanAdapter.h"
#include "ConstStringAdapter.h"
#include "Context.h"
#include "IntegerAdapter.h"
#include "SizeAdapter.h"
#include "StringAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "AssOpBuilder.h"
#include "BinOpBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "UniOpBuilder.h"
#include "BasicBlockBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/RT/BooleanDesc.h>
#include <Fabric/Base/Util/Format.h>

namespace Fabric
{
  namespace CG
  {
    BooleanAdapter::BooleanAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::BooleanDesc> const &booleanDesc )
      : SimpleAdapter( manager, booleanDesc )
      , m_booleanDesc( booleanDesc )
    {
    }
    
    llvm::Type const *BooleanAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      return llvm::Type::getInt1Ty( context->getLLVMContext() );
    }
    
    void BooleanAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
        
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      RC::ConstHandle<IntegerAdapter> integerAdapter = getManager()->getIntegerAdapter();
      integerAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<ConstStringAdapter> constStringAdapter = getManager()->getConstStringAdapter();
      constStringAdapter->llvmCompileToModule( moduleBuilder );
      
      static const bool buildFunctions = true;
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, stringAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *booleanRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::BasicBlock *trueBB = functionBuilder.createBasicBlock( "true" );
          llvm::BasicBlock *falseBB = functionBuilder.createBasicBlock( "false" );
          basicBlockBuilder->CreateCondBr( booleanRValue, trueBB, falseBB );
          
          basicBlockBuilder->SetInsertPoint( trueBB );
          ExprValue trueExprValue( constStringAdapter, USAGE_RVALUE, context, constStringAdapter->llvmConst( basicBlockBuilder, "true", 4 ) );
          llvm::Value *trueStringRValue = stringAdapter->llvmCast( basicBlockBuilder, trueExprValue );
          stringAdapter->llvmAssign( basicBlockBuilder, stringLValue, trueStringRValue );
          stringAdapter->llvmDispose( basicBlockBuilder, trueStringRValue );
          basicBlockBuilder->CreateRetVoid();
          
          basicBlockBuilder->SetInsertPoint( falseBB );
          ExprValue falseExprValue( constStringAdapter, USAGE_RVALUE, context, constStringAdapter->llvmConst( basicBlockBuilder, "false", 5 ) );
          llvm::Value *falseStringRValue = stringAdapter->llvmCast( basicBlockBuilder, falseExprValue );
          stringAdapter->llvmAssign( basicBlockBuilder, stringLValue, falseStringRValue );
          stringAdapter->llvmDispose( basicBlockBuilder, falseStringRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      /*
      // [pzion 20110202] It's unclear that we want booleans to automatically cast to numerics
      
      {
        std::string name = constructOverloadName( byteDesc, ExprType( this, USAGE_RVALUE ) );
        ParamVector params;
        params.push_back( FunctionParam( "booleanRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( byteDesc, USAGE_RVALUE ), params );
        llvm::Value *booleanRValue = functionBuilder[0];
        BasicBlockBuilder basicBlockBuilder( functionBuilder );
        basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
        llvm::Value *byteRValue = basicBlockBuilder->CreateZExt( booleanRValue, llvmRType() );
        basicBlockBuilder->CreateRet( byteRValue );
      }

      {
        std::string name = constructOverloadName( integerDesc, ExprType( this, USAGE_RVALUE ) );
        ParamVector params;
        params.push_back( FunctionParam( "booleanRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( integerDesc, USAGE_RVALUE ), params );
        llvm::Value *booleanRValue = functionBuilder[0];
        BasicBlockBuilder basicBlockBuilder( functionBuilder );
        basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
        llvm::Value *integerRValue = basicBlockBuilder->CreateZExt( booleanRValue, llvmRType() );
        basicBlockBuilder->CreateRet( integerRValue );
      }
      
      {
        std::string name = constructOverloadName( scalarDesc, ExprType( this, USAGE_RVALUE ) );
        ParamVector params;
        params.push_back( FunctionParam( "booleanRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( scalarDesc, USAGE_RVALUE ), params );
        llvm::Value *booleanRValue = functionBuilder[0];
        BasicBlockBuilder basicBlockBuilder( functionBuilder );
        basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
        llvm::Value *scalarRValue = basicBlockBuilder->CreateUIToFP( booleanRValue, llvmRType() );
        basicBlockBuilder->CreateRet( scalarRValue );
      }
      */
      
      {
        UniOpBuilder functionBuilder( moduleBuilder, this, UNI_OP_BIT_NOT, this, USAGE_RVALUE );
        if ( buildFunctions )
        {
          llvm::Value *rValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *notRValue = basicBlockBuilder->CreateNot( rValue );
          basicBlockBuilder->CreateRet( notRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_BIT_OR, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateOr( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_BIT_AND, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateAnd( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_BIT_XOR, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateXor( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_EQ, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateICmpEQ( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_NE, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateICmpNE( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder, sizeAdapter,
          this, USAGE_RVALUE,
          "hash"
          );
        if ( buildFunctions )
        {
          llvm::Value *rValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateZExt(
              rValue,
              sizeAdapter->llvmRType( context )
              )
            );
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          integerAdapter,
          this, USAGE_RVALUE,
          "compare",
          "that", this, USAGE_RVALUE
          );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *lhsRValueAsInteger = basicBlockBuilder->CreateZExt( lhsRValue, integerAdapter->llvmRType( context ) );
          llvm::Value *rhsRValueAsInteger = basicBlockBuilder->CreateZExt( rhsRValue, integerAdapter->llvmRType( context ) );
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateSub(
              lhsRValueAsInteger,
              rhsRValueAsInteger
              )
            );
        }
      }
    }

    llvm::Constant *BooleanAdapter::llvmConst( RC::Handle<Context> const &context, bool value ) const
    {
      return llvm::ConstantInt::get( llvmRType( context ), value, false );
    }
    
    std::string BooleanAdapter::toString( void const *data ) const
    {
      return _( m_booleanDesc->getValue(data) );
    }
    
    llvm::Constant *BooleanAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmConst( basicBlockBuilder.getContext(), m_booleanDesc->getValue( m_booleanDesc->getDefaultData() ) );
    }
  };
};
