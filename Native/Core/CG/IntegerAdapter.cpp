/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "IntegerAdapter.h"
#include "BooleanAdapter.h"
#include "ByteAdapter.h"
#include "SizeAdapter.h"
#include "FloatAdapter.h"
#include "ConstStringAdapter.h"
#include "StringAdapter.h"
#include "OpaqueAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "BinOpBuilder.h"
#include "UniOpBuilder.h"
#include "BasicBlockBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/RT/NumericDesc.h>
#include <Fabric/Core/RT/IntegerImpl.h>
#include <Fabric/Base/Util/Format.h>

namespace Fabric
{
  namespace CG
  {
    IntegerAdapter::IntegerAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::NumericDesc> const &integerDesc )
      : SimpleAdapter( manager, integerDesc )
      , m_integerDesc( integerDesc )
    {
    }
    
    llvm::Type const *IntegerAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      return llvm::Type::getInt32Ty( context->getLLVMContext() );
    }

    void IntegerAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
      
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<ByteAdapter> byteAdapter = getManager()->getByteAdapter();
      byteAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<FloatAdapter> fp32Adapter = getManager()->getFloat32Adapter();
      fp32Adapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<FloatAdapter> fp64Adapter = getManager()->getFloat64Adapter();
      fp64Adapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<OpaqueAdapter> dataAdapter = getManager()->getDataAdapter();
      dataAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<ConstStringAdapter> constStringAdapter = getManager()->getConstStringAdapter();
      constStringAdapter->llvmCompileToModule( moduleBuilder );
      
      static const bool buildFunctions = true;
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, booleanAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *booleanLValue = functionBuilder[0];
          llvm::Value *integerRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *booleanRValue = basicBlockBuilder->CreateIsNotNull( integerRValue );
          booleanAdapter->llvmAssign( basicBlockBuilder, booleanLValue, booleanRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, byteAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *byteLValue = functionBuilder[0];
          llvm::Value *integerRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *byteRValue = basicBlockBuilder->CreateTrunc( integerRValue, byteAdapter->llvmRType( context ) );
          byteAdapter->llvmAssign( basicBlockBuilder, byteLValue, byteRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, sizeAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *sizeLValue = functionBuilder[0];
          llvm::Value *integerRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *sizeRValue = basicBlockBuilder->CreateSExt( integerRValue, sizeAdapter->llvmRType( context ) );
          sizeAdapter->llvmAssign( basicBlockBuilder, sizeLValue, sizeRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, fp32Adapter, this, ConstructorBuilder::MedLowCost );
        if ( buildFunctions )
        {
          llvm::Value *scalarLValue = functionBuilder[0];
          llvm::Value *integerRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *scalarRValue = basicBlockBuilder->CreateSIToFP( integerRValue, fp32Adapter->llvmRType( context ) );
          fp32Adapter->llvmAssign( basicBlockBuilder, scalarLValue, scalarRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, fp64Adapter, this, ConstructorBuilder::MedHighCost );
        if ( buildFunctions )
        {
          llvm::Value *scalarLValue = functionBuilder[0];
          llvm::Value *integerRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *scalarRValue = basicBlockBuilder->CreateSIToFP( integerRValue, fp64Adapter->llvmRType( context ) );
          fp64Adapter->llvmAssign( basicBlockBuilder, scalarLValue, scalarRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, stringAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *integerRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *integerLValue = llvmRValueToLValue( basicBlockBuilder, integerRValue );
          stringAdapter->llvmCallCast( basicBlockBuilder, this, integerLValue, stringLValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        UniOpBuilder functionBuilder( moduleBuilder, this, UNI_OP_POS, this, USAGE_RVALUE );
        if ( buildFunctions )
        {
          llvm::Value *rValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          basicBlockBuilder->CreateRet( rValue );
        }
      }
      
      {
        UniOpBuilder functionBuilder( moduleBuilder, this, UNI_OP_NEG, this, USAGE_RVALUE );
        if ( buildFunctions )
        {
          llvm::Value *rValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *negRValue = basicBlockBuilder->CreateNeg( rValue );
          basicBlockBuilder->CreateRet( negRValue );
        }
      }
      
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
        UniOpBuilder functionBuilder( moduleBuilder, this, UNI_OP_PRE_INC, this, USAGE_LVALUE );
        if ( buildFunctions )
        {
          llvm::Value *lValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *preRValue = basicBlockBuilder->CreateLoad( lValue );
          llvm::Value *postRValue = basicBlockBuilder->CreateAdd( preRValue, llvm::ConstantInt::get( llvmRType( context ), 1, false ) );
          basicBlockBuilder->CreateStore( postRValue, lValue );
          basicBlockBuilder->CreateRet( postRValue );
        }
      }
      
      {
        UniOpBuilder functionBuilder( moduleBuilder, this, UNI_OP_PRE_DEC, this, USAGE_LVALUE );
        if ( buildFunctions )
        {
          llvm::Value *lValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *preRValue = basicBlockBuilder->CreateLoad( lValue );
          llvm::Value *postRValue = basicBlockBuilder->CreateSub( preRValue, llvm::ConstantInt::get( llvmRType( context ), 1, false ) );
          basicBlockBuilder->CreateStore( postRValue, lValue );
          basicBlockBuilder->CreateRet( postRValue );
        }
      }
      
      {
        UniOpBuilder functionBuilder( moduleBuilder, this, UNI_OP_POST_INC, this, USAGE_LVALUE );
        if ( buildFunctions )
        {
          llvm::Value *lValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *preRValue = basicBlockBuilder->CreateLoad( lValue );
          llvm::Value *postRValue = basicBlockBuilder->CreateAdd( preRValue, llvm::ConstantInt::get( llvmRType( context ), 1, false ) );
          basicBlockBuilder->CreateStore( postRValue, lValue );
          basicBlockBuilder->CreateRet( preRValue );
        }
      }
      
      {
        UniOpBuilder functionBuilder( moduleBuilder, this, UNI_OP_POST_DEC, this, USAGE_LVALUE );
        if ( buildFunctions )
        {
          llvm::Value *lValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *preRValue = basicBlockBuilder->CreateLoad( lValue );
          llvm::Value *postRValue = basicBlockBuilder->CreateSub( preRValue, llvm::ConstantInt::get( llvmRType( context ), 1, false ) );
          basicBlockBuilder->CreateStore( postRValue, lValue );
          basicBlockBuilder->CreateRet( preRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_ADD, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateAdd( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_SUB, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateSub( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_MUL, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateMul( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_DIV, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          
          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          llvm::BasicBlock *nonZeroBB = functionBuilder.createBasicBlock( "nonZero" );
          llvm::BasicBlock *zeroBB = functionBuilder.createBasicBlock( "zero" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *isNonZeroRValue = basicBlockBuilder->CreateIsNotNull( rhsRValue );
          basicBlockBuilder->CreateCondBr( isNonZeroRValue, nonZeroBB, zeroBB );
          
          basicBlockBuilder->SetInsertPoint( nonZeroBB );
          llvm::Value *resultRValue = basicBlockBuilder->CreateSDiv( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
          
          basicBlockBuilder->SetInsertPoint( zeroBB );
          std::string errorMsg = "KL: "+getUserName()+" division by zero";
          ExprValue errorExprValue( constStringAdapter, USAGE_RVALUE, context, constStringAdapter->llvmConst( basicBlockBuilder, errorMsg ) );
          llvm::Value *errorStringRValue = stringAdapter->llvmCast( basicBlockBuilder, errorExprValue );
          stringAdapter->llvmReport( basicBlockBuilder, errorStringRValue );
          stringAdapter->llvmDispose( basicBlockBuilder, errorStringRValue );
          llvm::Value *defaultRValue = llvmDefaultRValue( basicBlockBuilder );
          basicBlockBuilder->CreateRet( defaultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_MOD, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          
          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          llvm::BasicBlock *nonZeroBB = functionBuilder.createBasicBlock( "nonZero" );
          llvm::BasicBlock *zeroBB = functionBuilder.createBasicBlock( "zero" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *isNonZeroRValue = basicBlockBuilder->CreateIsNotNull( rhsRValue );
          basicBlockBuilder->CreateCondBr( isNonZeroRValue, nonZeroBB, zeroBB );
          
          basicBlockBuilder->SetInsertPoint( nonZeroBB );
          llvm::Value *resultRValue = basicBlockBuilder->CreateSRem( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
          
          basicBlockBuilder->SetInsertPoint( zeroBB );
          std::string errorMsg = "KL: "+getUserName()+" division by zero";
          ExprValue errorExprValue( constStringAdapter, USAGE_RVALUE, context, constStringAdapter->llvmConst( basicBlockBuilder, errorMsg ) );
          llvm::Value *errorStringRValue = stringAdapter->llvmCast( basicBlockBuilder, errorExprValue );
          stringAdapter->llvmReport( basicBlockBuilder, errorStringRValue );
          stringAdapter->llvmDispose( basicBlockBuilder, errorStringRValue );
          llvm::Value *defaultRValue = llvmDefaultRValue( basicBlockBuilder );
          basicBlockBuilder->CreateRet( defaultRValue );
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
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_SHL, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateShl( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_SHR, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateAShr( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, booleanAdapter, BIN_OP_EQ, this, this );
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
        BinOpBuilder functionBuilder( moduleBuilder, booleanAdapter, BIN_OP_NE, this, this );
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
        BinOpBuilder functionBuilder( moduleBuilder, booleanAdapter, BIN_OP_GT, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateICmpSGT( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, booleanAdapter, BIN_OP_GE, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateICmpSGE( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, booleanAdapter, BIN_OP_LT, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateICmpSLT( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, booleanAdapter, BIN_OP_LE, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateICmpSLE( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          sizeAdapter,
          this, USAGE_RVALUE,
          "dataSize"
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *dataSizeRValue = llvm::ConstantInt::get( sizeAdapter->llvmRType( context ), getDesc()->getAllocSize() );
          basicBlockBuilder->CreateRet( dataSizeRValue );
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          dataAdapter,
          this, USAGE_LVALUE,
          "data"
          );
        if ( buildFunctions )
        {
          llvm::Value *thisLValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          basicBlockBuilder->CreateRet( basicBlockBuilder->CreatePointerCast( thisLValue, dataAdapter->llvmRType( context ) ) );
        }
      }
     
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          sizeAdapter,
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
          this,
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
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateSub(
              lhsRValue,
              rhsRValue
              )
            );
        }
      }
    }
    
    llvm::Constant *IntegerAdapter::llvmConst( RC::Handle<Context> const &context, int32_t value ) const
    {
      return llvm::ConstantInt::get( llvmRawType( context ), value, true );
    }

    std::string IntegerAdapter::toString( void const *data ) const
    {
      return m_integerDesc->toString( data );
    }
    
    llvm::Constant *IntegerAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<RT::SI32Impl> si32Impl = RC::ConstHandle<RT::SI32Impl>::StaticCast( m_integerDesc->getImpl() );
      return llvmConst( basicBlockBuilder.getContext(), si32Impl->getValue( si32Impl->getDefaultData() ) );
    }
  };
};
