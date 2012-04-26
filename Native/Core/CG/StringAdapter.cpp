/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "StringAdapter.h"
#include "BooleanAdapter.h"
#include "ByteAdapter.h"
#include "IntegerAdapter.h"
#include "SizeAdapter.h"
#include "FloatAdapter.h"
#include "OpaqueAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "AssOpBuilder.h"
#include "BinOpBuilder.h"
#include "InternalFunctionBuilder.h"
#include "BasicBlockBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/RT/StringImpl.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Intrinsics.h>
#include <llvm/GlobalVariable.h>

namespace Fabric
{
  namespace CG
  {
    StringAdapter::StringAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::StringDesc> const &stringDesc )
      : Adapter( manager, stringDesc, FL_PASS_BY_REFERENCE )
      , m_stringDesc( stringDesc )
    {
    }
    
    llvm::Type *StringAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::Type *llvmSizeTy = llvmSizeType( context );
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
      
      std::vector<llvm::Type *> memberLLVMTypes;
      memberLLVMTypes.push_back( llvmSizeTy ); // refCount
      memberLLVMTypes.push_back( llvmSizeTy ); // allocSize
      memberLLVMTypes.push_back( llvmSizeTy ); // length
      memberLLVMTypes.push_back( llvm::ArrayType::get( llvm::Type::getInt8Ty( llvmContext ), 0 ) );
      llvm::Type *implType = llvm::StructType::create( llvmContext, memberLLVMTypes, getCodeName() + ".Bits", true );

      return implType->getPointerTo();
    }
    
    llvm::Type *StringAdapter::getLLVMImplType( RC::Handle<Context> const &context ) const
    {
      return static_cast<llvm::PointerType *>( llvmRawType( context ) )->getElementType();
    }
    
    void StringAdapter::llvmCallCast( BasicBlockBuilder &basicBlockBuilder, RC::ConstHandle<Adapter> const &adapter, llvm::Value *srcLValue, llvm::Value *dstLValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      
      llvm::Type *int8PtrTy = basicBlockBuilder->getInt8PtrTy();
      
      std::vector< llvm::Type * > argTypes;
      argTypes.push_back( int8PtrTy );
      argTypes.push_back( adapter->llvmLType( context ) );
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__String__Cast", funcType );

      std::vector< llvm::Value * > args;
      args.push_back( adapter->llvmAdapterPtr( basicBlockBuilder ) );
      args.push_back( srcLValue );
      args.push_back( dstLValue );
      basicBlockBuilder->CreateCall( func, args );
    }
    
    void StringAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
        
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      llvm::Type *implType = getLLVMImplType( context );
  
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<IntegerAdapter> integerAdapter = getManager()->getIntegerAdapter();
      integerAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<OpaqueAdapter> dataAdapter = getManager()->getDataAdapter();
      dataAdapter->llvmCompileToModule( moduleBuilder );
            
      static const bool buildFunctions = true;
     
      {
        std::vector< llvm::Type * > argTypes;
        std::string name = "report";
        ParamVector params(
          FunctionParam(
            "stringRValue",
            this,
            USAGE_RVALUE
            )
          );
        ExprTypeVector exprTypes = params.getTypes();
        FunctionBuilder functionBuilder(
          moduleBuilder,
          CG::FunctionPencilKey( name ),
          CG::FunctionDefaultSymbolName(
            name,
            exprTypes
            ),
          CG::FunctionFullDesc(
            0,
            name,
            exprTypes
            ),
          0,
          params,
          0
          );

        BasicBlockBuilder basicBlockBuilder( functionBuilder );

        llvm::Value *rValue = functionBuilder[0];

        llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
        llvm::BasicBlock *notNullBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "notNull" );
        llvm::BasicBlock *doneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "done" );
        
        basicBlockBuilder->SetInsertPoint( entryBB );
        llvm::Value *bitsLValue = basicBlockBuilder->CreateLoad( rValue );
        llvm::Value *isNotNullRValue = basicBlockBuilder->CreateIsNotNull( bitsLValue );
        basicBlockBuilder->CreateCondBr( isNotNullRValue, notNullBB, doneBB );
        
        basicBlockBuilder->SetInsertPoint( notNullBB );
        llvmReport( basicBlockBuilder, rValue );
        basicBlockBuilder->CreateBr( doneBB );
        
        basicBlockBuilder->SetInsertPoint( doneBB );
        basicBlockBuilder->CreateRetVoid();

        moduleBuilder.getScope().put( "report", functionBuilder.getPencil() );
      }
 
      {
        ConstructorBuilder functionBuilder( moduleBuilder, booleanAdapter, this, ConstructorBuilder::HighCost );
        if ( buildFunctions )
        {
          llvm::Value *booleanLValue = functionBuilder[0];
          llvm::Value *stringRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *lengthRValue = llvmCallLength( basicBlockBuilder, stringRValue );
          llvm::Value *booleanRValue = basicBlockBuilder->CreateICmpNE( lengthRValue, sizeAdapter->llvmConst( context, 0 ) );
          basicBlockBuilder->CreateStore( booleanRValue, booleanLValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      llvm::Function *assignAddFunction = 0;
      {
        std::vector< llvm::Type * > argTypes;
        argTypes.push_back( llvmLType( context ) );
        argTypes.push_back( llvmRType( context ) );
        llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
        llvm::Constant *func = moduleBuilder->getOrInsertFunction( "__String__Append", funcType ); 

        AssOpBuilder functionBuilder( moduleBuilder, this, ASSIGN_OP_ADD, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsLValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          std::vector< llvm::Value * > args;
          args.push_back( lhsLValue );
          args.push_back( rhsRValue );
          basicBlockBuilder->CreateCall( func, args );
          basicBlockBuilder->CreateRetVoid();
          assignAddFunction = functionBuilder.getLLVMFunction();
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
          llvm::Value *resultLValue = functionBuilder.getScope().llvmGetReturnLValue();
          llvmAssign( basicBlockBuilder, resultLValue, lhsRValue );
          basicBlockBuilder->CreateCall2( assignAddFunction, resultLValue, rhsRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          sizeAdapter,
          this, USAGE_RVALUE,
          "length"
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *rValue = functionBuilder[0];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *nonNullBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "nonNull" );
          llvm::BasicBlock *nullBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "null" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *bitsLValue = basicBlockBuilder->CreateLoad( rValue );
          basicBlockBuilder->CreateCondBr( basicBlockBuilder->CreateIsNotNull( bitsLValue ), nonNullBB, nullBB );
          
          basicBlockBuilder->SetInsertPoint( nonNullBB );
          llvm::Value *lengthLValue = basicBlockBuilder->CreateStructGEP( bitsLValue, 2 );
          llvm::Value *lengthRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, lengthLValue );
          basicBlockBuilder->CreateRet( lengthRValue );
          
          basicBlockBuilder->SetInsertPoint( nullBB );
          basicBlockBuilder->CreateRet( sizeAdapter->llvmConst( context, 0 ) );
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
          llvm::Value *thisRValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *lengthRValue = llvmCallLength( basicBlockBuilder, thisRValue );
          llvm::Value *dataSizeRValue = basicBlockBuilder->CreateAdd( lengthRValue, llvm::ConstantInt::get( lengthRValue->getType(), 1 ) );
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
          llvm::Value *thisRValue = functionBuilder[0];
          
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::BasicBlock *nullBB = functionBuilder.createBasicBlock( "null" );
          llvm::BasicBlock *nonNullBB = functionBuilder.createBasicBlock( "nonNull" );
          llvm::Value *bitsLValue = basicBlockBuilder->CreateLoad( thisRValue );
          basicBlockBuilder->CreateCondBr( basicBlockBuilder->CreateIsNull( bitsLValue ), nullBB, nonNullBB );
          
          basicBlockBuilder->SetInsertPoint( nullBB );
          basicBlockBuilder->CreateRet( basicBlockBuilder->CreatePointerCast( bitsLValue, dataAdapter->llvmRType( context ) ) );
          
          basicBlockBuilder->SetInsertPoint( nonNullBB );
          basicBlockBuilder->CreateRet( basicBlockBuilder->CreatePointerCast( basicBlockBuilder->CreateConstGEP1_32( basicBlockBuilder->CreateConstGEP2_32( basicBlockBuilder->CreateStructGEP( bitsLValue, 3 ), 0, 0 ), 0 ), dataAdapter->llvmRType( context ) ) );
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
          llvm::Value *thisRValue = functionBuilder[0];
          llvm::Value *otherRValue = functionBuilder[1];
          
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          llvm::BasicBlock *checkPointersBB = functionBuilder.createBasicBlock( "checkPointers" );
          llvm::BasicBlock *thisIsNullBB = functionBuilder.createBasicBlock( "thisIsNull" );
          llvm::BasicBlock *thisIsNotNullBB = functionBuilder.createBasicBlock( "thisIsNotNull" );
          llvm::BasicBlock *shallowBB = functionBuilder.createBasicBlock( "shallow" );
          llvm::BasicBlock *deepBB = functionBuilder.createBasicBlock( "deep" );
          llvm::BasicBlock *checkBB = functionBuilder.createBasicBlock( "check" );
          llvm::BasicBlock *checkSelfLengthBB = functionBuilder.createBasicBlock( "checkSelfLength" );
          llvm::BasicBlock *verifyOtherLengthBB = functionBuilder.createBasicBlock( "verifyOtherLength" );
          llvm::BasicBlock *checkOtherLengthBB = functionBuilder.createBasicBlock( "checkOtherLength" );
          llvm::BasicBlock *checkCharsBB = functionBuilder.createBasicBlock( "checkChars" );
          llvm::BasicBlock *checkCharsLTBB = functionBuilder.createBasicBlock( "checkCharsLT" );
          llvm::BasicBlock *checkCharsGTBB = functionBuilder.createBasicBlock( "checkCharsGT" );
          llvm::BasicBlock *nextBB = functionBuilder.createBasicBlock( "next" );
          llvm::BasicBlock *eqBB = functionBuilder.createBasicBlock( "eq" );
          llvm::BasicBlock *ltBB = functionBuilder.createBasicBlock( "lt" );
          llvm::BasicBlock *gtBB = functionBuilder.createBasicBlock( "gt" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *thisBitsLValue = basicBlockBuilder->CreateLoad( thisRValue );
          llvm::Value *otherBitsLValue = basicBlockBuilder->CreateLoad( otherRValue );
          basicBlockBuilder->CreateBr( checkPointersBB );
          
          basicBlockBuilder->SetInsertPoint( checkPointersBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateIsNull(thisBitsLValue),
            thisIsNullBB,
            thisIsNotNullBB
            );
            
          basicBlockBuilder->SetInsertPoint( thisIsNullBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateIsNull(otherBitsLValue),
            eqBB,
            ltBB
            );
            
          basicBlockBuilder->SetInsertPoint( thisIsNotNullBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateIsNull(otherBitsLValue),
            gtBB,
            shallowBB
            );

          basicBlockBuilder->SetInsertPoint( shallowBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpEQ( thisBitsLValue, otherBitsLValue ),
            eqBB,
            deepBB
            );
          
          basicBlockBuilder->SetInsertPoint( deepBB );
          llvm::Value *thisLengthRValue = basicBlockBuilder->CreateLoad( basicBlockBuilder->CreateStructGEP( thisBitsLValue, 2, "thisLengthPtr" ), "thisLength" );
          llvm::Value *thisCStrRValue = basicBlockBuilder->CreateConstGEP2_32( basicBlockBuilder->CreateStructGEP( thisBitsLValue, 3 ), 0, 0, "thisCStr" );
          llvm::Value *otherLengthRValue = basicBlockBuilder->CreateLoad( basicBlockBuilder->CreateStructGEP( otherBitsLValue, 2, "otherLengthPtr" ), "otherLength" );
          llvm::Value *otherCStrRValue = basicBlockBuilder->CreateConstGEP2_32( basicBlockBuilder->CreateStructGEP( otherBitsLValue, 3 ), 0, 0, "otherCStr" );
          llvm::Value *indexLValue = basicBlockBuilder->CreateAlloca( sizeAdapter->llvmRType( context ), sizeAdapter->llvmConst( context, 1 ), "indexPtr" );
          basicBlockBuilder->CreateStore( sizeAdapter->llvmConst( context, 0 ), indexLValue );
          basicBlockBuilder->CreateBr( checkBB );
          
          basicBlockBuilder->SetInsertPoint( checkBB );
          llvm::Value *indexRValue = basicBlockBuilder->CreateLoad( indexLValue, "index" );
          basicBlockBuilder->CreateBr( checkSelfLengthBB );
          
          basicBlockBuilder->SetInsertPoint( checkSelfLengthBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpULT( indexRValue, thisLengthRValue ),
            checkOtherLengthBB,
            verifyOtherLengthBB
            );
          
          basicBlockBuilder->SetInsertPoint( verifyOtherLengthBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpULT( indexRValue, otherLengthRValue ),
            ltBB,
            eqBB
            );
          
          basicBlockBuilder->SetInsertPoint( checkOtherLengthBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpULT( indexRValue, otherLengthRValue ),
            checkCharsBB,
            gtBB
            );
            
          basicBlockBuilder->SetInsertPoint( checkCharsBB );
          llvm::Value *thisCharPtr = basicBlockBuilder->CreateGEP( thisCStrRValue, indexRValue, "thisCharPtr" );
          llvm::Value *thisChar = basicBlockBuilder->CreateLoad( thisCharPtr, "thisChar" );
          llvm::Value *otherCharPtr = basicBlockBuilder->CreateGEP( otherCStrRValue, indexRValue, "otherCharPtr" );
          llvm::Value *otherChar = basicBlockBuilder->CreateLoad( otherCharPtr, "otherChar" );
          basicBlockBuilder->CreateBr( checkCharsLTBB );
          
          basicBlockBuilder->SetInsertPoint( checkCharsLTBB );
          basicBlockBuilder->CreateCondBr( basicBlockBuilder->CreateICmpULT( thisChar, otherChar ), ltBB, checkCharsGTBB );

          basicBlockBuilder->SetInsertPoint( checkCharsGTBB );
          basicBlockBuilder->CreateCondBr( basicBlockBuilder->CreateICmpUGT( thisChar, otherChar ), gtBB, nextBB );

          basicBlockBuilder->SetInsertPoint( nextBB );
          basicBlockBuilder->CreateStore(
            basicBlockBuilder->CreateAdd(
              indexRValue,
              sizeAdapter->llvmConst( context, 1 )
              ),
            indexLValue
            );
          basicBlockBuilder->CreateBr( checkBB );
          
          basicBlockBuilder->SetInsertPoint( eqBB );
          basicBlockBuilder->CreateRet( integerAdapter->llvmConst( context, 0 ) );
          
          basicBlockBuilder->SetInsertPoint( ltBB );
          basicBlockBuilder->CreateRet( integerAdapter->llvmConst( context, -1 ) );
          
          basicBlockBuilder->SetInsertPoint( gtBB );
          basicBlockBuilder->CreateRet( integerAdapter->llvmConst( context, 1 ) );
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
          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          llvm::BasicBlock *notNullBB = functionBuilder.createBasicBlock( "notNull" );
          llvm::BasicBlock *loopCheckBB = functionBuilder.createBasicBlock( "loopCheck" );
          llvm::BasicBlock *loopStepBB = functionBuilder.createBasicBlock( "loopStep" );
          llvm::BasicBlock *doneBB = functionBuilder.createBasicBlock( "done" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *resultLValue = sizeAdapter->llvmAlloca( basicBlockBuilder, "result" );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, resultLValue, sizeAdapter->llvmConst( context, 5381 ) );
          llvm::Value *bitsLValue = basicBlockBuilder->CreateLoad( rValue );
          llvm::Value *isNullRValue = basicBlockBuilder->CreateIsNull(bitsLValue);
          basicBlockBuilder->CreateCondBr( isNullRValue, doneBB, notNullBB );
          
          basicBlockBuilder->SetInsertPoint( notNullBB );
          llvm::Value *cStrRValue = basicBlockBuilder->CreateConstGEP2_32( basicBlockBuilder->CreateStructGEP( bitsLValue, 3 ), 0, 0, "cStr" );
          llvm::Value *lengthRValue = llvmCallLength( basicBlockBuilder, rValue );
          llvm::Value *indexLValue = sizeAdapter->llvmAlloca( basicBlockBuilder, "index" );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, indexLValue, sizeAdapter->llvmConst( context, 0 ) );
          basicBlockBuilder->CreateBr( loopCheckBB );
          
          basicBlockBuilder->SetInsertPoint( loopCheckBB );
          llvm::Value *indexRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, indexLValue );
          llvm::Value *cmpRValue = basicBlockBuilder->CreateICmpULT( indexRValue, lengthRValue );
          basicBlockBuilder->CreateCondBr( cmpRValue, loopStepBB, doneBB );
          
          basicBlockBuilder->SetInsertPoint( loopStepBB );
          llvm::Value *charLValue = basicBlockBuilder->CreateGEP( cStrRValue, indexRValue, "charLValue" );
          llvm::Value *charRValue = basicBlockBuilder->CreateLoad( charLValue, "charRValue" );
          llvm::Value *resultRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue );
          sizeAdapter->llvmDefaultAssign(
            basicBlockBuilder,
            resultLValue,
            basicBlockBuilder->CreateXor(
              basicBlockBuilder->CreateMul(
                resultRValue,
                sizeAdapter->llvmConst( context, 33 )
                ),
              basicBlockBuilder->CreateZExt(
                charRValue,
                sizeAdapter->llvmRType( context )
                )
              )
            );
          sizeAdapter->llvmDefaultAssign(
            basicBlockBuilder,
            indexLValue,
            basicBlockBuilder->CreateAdd(
              indexRValue,
              sizeAdapter->llvmConst( context, 1 )
              )
            );
          basicBlockBuilder->CreateBr( loopCheckBB );
          
          basicBlockBuilder->SetInsertPoint( doneBB );
          resultRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue );
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
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateICmpEQ(
              llvmCompare( basicBlockBuilder, lhsRValue, rhsRValue ),
              integerAdapter->llvmConst( context, 0 )
              )
            );
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
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateICmpNE(
              llvmCompare( basicBlockBuilder, lhsRValue, rhsRValue ),
              integerAdapter->llvmConst( context, 0 )
              )
            );
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
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateICmpSGT(
              llvmCompare( basicBlockBuilder, lhsRValue, rhsRValue ),
              integerAdapter->llvmConst( context, 0 )
              )
            );
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
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateICmpSGE(
              llvmCompare( basicBlockBuilder, lhsRValue, rhsRValue ),
              integerAdapter->llvmConst( context, 0 )
              )
            );
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
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateICmpSLT(
              llvmCompare( basicBlockBuilder, lhsRValue, rhsRValue ),
              integerAdapter->llvmConst( context, 0 )
              )
            );
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
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateICmpSLE(
              llvmCompare( basicBlockBuilder, lhsRValue, rhsRValue ),
              integerAdapter->llvmConst( context, 0 )
              )
            );
        }
      }
    }
    
    void StringAdapter::Cast( Adapter const *adapter, void const *srcLValue, void *dstLValue )
    {
      std::string string = adapter->toString( srcLValue );
      RT::StringImpl::SetValue( string.data(), string.length(), dstLValue );
    }
    
    void StringAdapter::Append( void *dstLValue, void const *srcLValue )
    {
      RT::StringImpl::Append( dstLValue, srcLValue );
    }
    
    void StringAdapter::Throw( void const *lValue )
    {
      throw Exception( RT::StringImpl::GetValueData( lValue ), RT::StringImpl::GetValueLength( lValue ) );
    }
    
    void StringAdapter::llvmReport( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();

      std::vector< llvm::Type * > argTypes;
      argTypes.push_back( llvm::Type::getInt8PtrTy( context->getLLVMContext() ) );
      argTypes.push_back( sizeAdapter->llvmRType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );
      
      llvm::Function *llvmReportFunction = llvm::cast<llvm::Function>(
        basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "report", funcType, attrListPtr )
        );

      llvm::Value *bitsLValue = basicBlockBuilder->CreateLoad( rValue );
      llvm::Value *dataLValue = basicBlockBuilder->CreateStructGEP( bitsLValue, 3 );
      llvm::Value *dataRValue = basicBlockBuilder->CreateConstGEP2_32( dataLValue, 0, 0 );
      llvm::Value *lengthLValue = basicBlockBuilder->CreateStructGEP( bitsLValue, 2 );
      llvm::Value *lengthRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, lengthLValue );
      basicBlockBuilder->CreateCall2( llvmReportFunction, dataRValue, lengthRValue );
    }
    
    void StringAdapter::llvmThrow( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      std::vector< llvm::Type * > argTypes;
      argTypes.push_back( llvmRType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      llvm::Constant *func = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__String__Throw", funcType ); 
      basicBlockBuilder->CreateCall( func, rValue );
    }
    
    void *StringAdapter::llvmResolveExternalFunction( std::string const &functionName ) const
    {
      if ( functionName == "__String__Append" )
        return (void *)&StringAdapter::Append;
      else if ( functionName == "__String__Cast" )
        return (void *)&StringAdapter::Cast;
      else if ( functionName == "__String__Throw" )
        return (void *)&StringAdapter::Throw;
      else return Adapter::llvmResolveExternalFunction( functionName );
    }

    void StringAdapter::llvmInit( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      llvm::PointerType *rawType = static_cast<llvm::PointerType *>( llvmRawType( basicBlockBuilder.getContext() ) );
      basicBlockBuilder->CreateStore(
        llvm::ConstantPointerNull::get( rawType ),
        lValue
        );
    }
    
    void StringAdapter::llvmRetain( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *bitsLValue ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();

      std::vector<llvm::Type *> argTypes;
      argTypes.push_back( bitsLValue->getType() );
      llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );

      std::string name = "__"+getCodeName()+"_Retain";
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getFunction( name ) );
      if ( !func )
      {
        ModuleBuilder &mb = basicBlockBuilder.getModuleBuilder();
        
        func = llvm::cast<llvm::Function>( mb->getOrInsertFunction( name, funcType, attrListPtr ) ); 
        func->setLinkage( llvm::GlobalValue::PrivateLinkage );
        
        FunctionBuilder fb( mb, funcType, func );
        llvm::Argument *bitsLValue = fb[0];
        bitsLValue->setName( "bitsLValue" );
        bitsLValue->addAttr( llvm::Attribute::NoCapture );
        
        BasicBlockBuilder bbb( fb );

        llvm::BasicBlock *entryBB = fb.createBasicBlock( "entry" );
        llvm::BasicBlock *nonNullBB = fb.createBasicBlock( "nonNull" );
        llvm::BasicBlock *doneBB = fb.createBasicBlock( "done" );
        
        bbb->SetInsertPoint( entryBB );
        bbb->CreateCondBr(
          bbb->CreateIsNotNull( bitsLValue ),
          nonNullBB,
          doneBB
          );
        
        bbb->SetInsertPoint( nonNullBB );

        llvm::Value *oneRValue = sizeAdapter->llvmConst( context, 1 );
        llvm::Value *refCountLValue = bbb->CreateStructGEP( bitsLValue, 0 );
        bbb->CreateAtomicRMW(llvm::AtomicRMWInst::Add, refCountLValue, oneRValue, llvm::Monotonic );

        bbb->CreateBr( doneBB );

        bbb->SetInsertPoint( doneBB );
        bbb->CreateRetVoid();
      }

      std::vector<llvm::Value *> args;
      args.push_back( bitsLValue );
      basicBlockBuilder->CreateCall( func, args );
    }
    
    void StringAdapter::llvmRelease( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *bitsLValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();

      std::vector<llvm::Type *> argTypes;
      argTypes.push_back( bitsLValue->getType() );
      llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );

      std::string name = "__"+getCodeName()+"_Release";
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getFunction( name ) );
      if ( !func )
      {
        ModuleBuilder &mb = basicBlockBuilder.getModuleBuilder();
        
        func = llvm::cast<llvm::Function>( mb->getOrInsertFunction( name, funcType, attrListPtr ) ); 
        func->setLinkage( llvm::GlobalValue::PrivateLinkage );
        
        FunctionBuilder fb( mb, funcType, func );
        llvm::Argument *bitsLValue = fb[0];
        bitsLValue->setName( "bitsLValue" );
        bitsLValue->addAttr( llvm::Attribute::NoCapture );
        
        BasicBlockBuilder bbb( fb );

        llvm::BasicBlock *entryBB = fb.createBasicBlock( "entry" );
        llvm::BasicBlock *nonNullBB = fb.createBasicBlock( "nonNull" );
        llvm::BasicBlock *freeBB = fb.createBasicBlock( "free" );
        llvm::BasicBlock *doneBB = fb.createBasicBlock( "done" );

        bbb->SetInsertPoint( entryBB );
        bbb->CreateCondBr(
          bbb->CreateIsNotNull( bitsLValue ),
          nonNullBB,
          doneBB
          );

        bbb->SetInsertPoint( nonNullBB );
        llvm::Value *refCountLValue = bbb->CreateStructGEP( bitsLValue, 0 );
        llvm::Value *oneRValue = sizeAdapter->llvmConst( bbb.getContext(), 1 );
        llvm::Value *oldRefCountRValue = bbb->CreateAtomicRMW(llvm::AtomicRMWInst::Sub, refCountLValue, oneRValue, llvm::Monotonic );

        llvm::Value *shouldFreeRValue = bbb->CreateICmpEQ( oldRefCountRValue, oneRValue );
        bbb->CreateCondBr( shouldFreeRValue, freeBB, doneBB );

        bbb->SetInsertPoint( freeBB );
        llvmCallFree( bbb, bitsLValue );
        bbb->CreateBr( doneBB );

        bbb->SetInsertPoint( doneBB );
        bbb->CreateRetVoid();
      }

      std::vector<llvm::Value *> args;
      args.push_back( bitsLValue );
      basicBlockBuilder->CreateCall( func, args );
    }

    void StringAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      llvm::Value *oldBitsLValue = basicBlockBuilder->CreateLoad( dstLValue );
      llvm::Value *newBitsLValue = basicBlockBuilder->CreateLoad( srcRValue );
      llvmRetain( basicBlockBuilder, newBitsLValue );
      basicBlockBuilder->CreateStore( newBitsLValue, dstLValue );
      llvmRelease( basicBlockBuilder, oldBitsLValue );
    }

    void StringAdapter::llvmDisposeImpl( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      llvm::Value *bitsLValue = basicBlockBuilder->CreateLoad( lValue );
      llvmRelease( basicBlockBuilder, bitsLValue );
    }
    
    llvm::Value *StringAdapter::llvmCallLength( BasicBlockBuilder &basicBlockBuilder, llvm::Value *stringRValue ) const
    {
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      MethodBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        sizeAdapter,
        this, USAGE_RVALUE,
        "length"
        );
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), stringRValue );
    }
    
    void StringAdapter::llvmCallConcat( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lhsRValue, llvm::Value *rhsRValue, llvm::Value *dstLValue ) const
    {
      BinOpBuilder functionBuilder( basicBlockBuilder.getModuleBuilder(), this, BIN_OP_ADD, this, this );
      basicBlockBuilder->CreateCall3( functionBuilder.getLLVMFunction(), dstLValue, lhsRValue, rhsRValue );
    }
    
    std::string StringAdapter::toString( void const *data ) const
    {
      char const *stringData = m_stringDesc->getValueData( data );
      size_t stringLength = m_stringDesc->getValueLength( data );
      return _( stringData, stringLength, SIZE_MAX, '"' );
    }
    
    llvm::Constant *StringAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvm::ConstantPointerNull::get( static_cast<llvm::PointerType *>( llvmRawType( basicBlockBuilder.getContext() ) ) );
    }
      
    llvm::Constant *StringAdapter::llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmDefaultLValue( basicBlockBuilder );
    }

    llvm::Constant *StringAdapter::llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const
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

    llvm::Value *StringAdapter::llvmHash( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      MethodBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        sizeAdapter,
        this, USAGE_RVALUE,
        "hash"
        );
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), rValue );
    }
    
    llvm::Value *StringAdapter::llvmCompare( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lhsRValue, llvm::Value *rhsRValue ) const
    {
      RC::ConstHandle<IntegerAdapter> integerAdapter = getManager()->getIntegerAdapter();
      MethodBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        integerAdapter,
        this, USAGE_RVALUE,
        "compare",
        "that", this, USAGE_RVALUE
        );
      return basicBlockBuilder->CreateCall2( functionBuilder.getLLVMFunction(), lhsRValue, rhsRValue );
    }
  };
};
