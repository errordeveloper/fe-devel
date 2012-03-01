/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "VariableArrayAdapter.h"
#include "BooleanAdapter.h"
#include "IntegerAdapter.h"
#include "SizeAdapter.h"
#include "ConstStringAdapter.h"
#include "StringAdapter.h"
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

#include <Fabric/Core/CG/CompileOptions.h>
#include <Fabric/Core/RT/VariableArrayDesc.h>
#include <Fabric/Core/RT/VariableArrayImpl.h>
#include <Fabric/Core/RT/Impl.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Intrinsics.h>

namespace Fabric
{
  namespace CG
  {
    VariableArrayAdapter::VariableArrayAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::VariableArrayDesc> const &variableArrayDesc )
      : ArrayAdapter( manager, variableArrayDesc, FL_PASS_BY_REFERENCE )
      , m_variableArrayDesc( variableArrayDesc )
      , m_variableArrayImpl( variableArrayDesc->getImpl() )
      , m_memberAdapter( manager->getAdapter( variableArrayDesc->getMemberDesc() ) )
    {
    }
    
    llvm::Type const *VariableArrayAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::Type const *llvmSizeTy = llvmSizeType( context );
      
      std::vector<llvm::Type const *> memberLLVMTypes;
      memberLLVMTypes.push_back( llvmSizeTy ); // allocNumMembers
      memberLLVMTypes.push_back( llvmSizeTy ); // numMembers
      memberLLVMTypes.push_back( m_memberAdapter->llvmRawType( context )->getPointerTo() );
      return llvm::StructType::get( context->getLLVMContext(), memberLLVMTypes, true );
    }
    
    void VariableArrayAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
      
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      llvm::Type const *implType = llvmRawType( context );

      m_memberAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<IntegerAdapter> integerAdapter = getManager()->getIntegerAdapter();
      integerAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<OpaqueAdapter> dataAdapter = getManager()->getDataAdapter();
      dataAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<ConstStringAdapter> constStringAdapter = getManager()->getConstStringAdapter();
      constStringAdapter->llvmCompileToModule( moduleBuilder );
      
      moduleBuilder->addTypeName( getCodeName(), implType );
      
      static const bool buildFunctions = true;
      bool const guarded = moduleBuilder.getCompileOptions()->getGuarded();
      
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
          
          llvm::Value *arrayRValue = functionBuilder[0];

          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *arrayLValue = llvmRValueToLValue( basicBlockBuilder, arrayRValue );
          llvm::Value *sizeLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, SizeIndex );
          llvm::Value *sizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, sizeLValue );
          basicBlockBuilder->CreateRet( sizeRValue );
        }
      }
      
      {
        InternalFunctionBuilder functionBuilder(
          moduleBuilder,
          0, "__"+getCodeName()+"__DefaultAssign",
          "dst", this, CG::USAGE_LVALUE,
          "src", this, CG::USAGE_RVALUE,
          0
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *dstLValue = functionBuilder[0];
          llvm::Value *srcRValue = functionBuilder[1];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *disposeCheckBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "disposeCheckBB" );
          llvm::BasicBlock *disposeNextBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "disposeNextBB" );
          llvm::BasicBlock *disposeDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "disposeDoneBB" );
          llvm::BasicBlock *zeroBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "zeroBB" );
          llvm::BasicBlock *nonZeroBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "nonZeroBB" );
          llvm::BasicBlock *freeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "freeBB" );
          llvm::BasicBlock *copyCheckBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "copyCheckBB" );
          llvm::BasicBlock *copyNextBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "copyNextBB" );
          llvm::BasicBlock *copyDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "copyDoneBB" );
          llvm::BasicBlock *doneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "doneBB" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *srcLValue = llvmRValueToLValue( basicBlockBuilder, srcRValue );
          llvm::Value *dstSizeLValue = basicBlockBuilder->CreateStructGEP( dstLValue, SizeIndex );
          llvm::Value *dstSizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, dstSizeLValue );
          llvm::Value *srcSizeLValue = basicBlockBuilder->CreateStructGEP( srcLValue, SizeIndex );
          llvm::Value *srcSizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, srcSizeLValue );
          llvm::Value *dstAllocSizeLValue = basicBlockBuilder->CreateStructGEP( dstLValue, AllocSizeIndex );
          llvm::Value *dstMemberDatasLValue = basicBlockBuilder->CreateStructGEP( dstLValue, MemberDatasIndex );
          llvm::Value *dstMemberDatasRValue = basicBlockBuilder->CreateLoad( dstMemberDatasLValue );
          llvm::Value *indexLValue = sizeAdapter->llvmAlloca( basicBlockBuilder, "indexLValue" );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, indexLValue, srcSizeRValue );
          basicBlockBuilder->CreateBr( disposeCheckBB );
          
          basicBlockBuilder->SetInsertPoint( disposeCheckBB );
          llvm::Value *indexRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, indexLValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpULT( indexRValue, dstSizeRValue ),
            disposeNextBB,
            disposeDoneBB
            );
          
          basicBlockBuilder->SetInsertPoint( disposeNextBB );
          llvm::Value *dstMemberLValue = basicBlockBuilder->CreateGEP( dstMemberDatasRValue, indexRValue );
          m_memberAdapter->llvmDispose( basicBlockBuilder, dstMemberLValue );
          sizeAdapter->llvmDefaultAssign(
            basicBlockBuilder,
            indexLValue,
            basicBlockBuilder->CreateAdd(
              indexRValue,
              sizeAdapter->llvmConst( context, 1 )
              )
            );
          basicBlockBuilder->CreateBr( disposeCheckBB );
          
          basicBlockBuilder->SetInsertPoint( disposeDoneBB );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, dstSizeLValue, srcSizeRValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpEQ( srcSizeRValue, sizeAdapter->llvmConst( context, 0 ) ),
            zeroBB,
            nonZeroBB
            );
            
          basicBlockBuilder->SetInsertPoint( zeroBB );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, dstAllocSizeLValue, sizeAdapter->llvmConst( context, 0 ) );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateIsNotNull( dstMemberDatasRValue ),
            freeBB,
            doneBB
            );
           
          basicBlockBuilder->SetInsertPoint( freeBB );
          llvmCallFree(
            basicBlockBuilder,
            dstMemberDatasRValue
            );
          basicBlockBuilder->CreateStore(
            llvm::ConstantPointerNull::get( static_cast<llvm::PointerType const *>( dstMemberDatasRValue->getType() ) ), dstMemberDatasLValue );
          basicBlockBuilder->CreateBr( doneBB );
           
          basicBlockBuilder->SetInsertPoint( nonZeroBB );
          llvm::Value *dstAllocSizeRValue = llvmCallMax(
            basicBlockBuilder,
            llvmCallNextPowTwoMinusOne(
              basicBlockBuilder,
              srcSizeRValue
              ),
            sizeAdapter->llvmConst( context, 15 )
            );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, dstAllocSizeLValue, dstAllocSizeRValue );
          llvm::Value *allocSizeRValue = basicBlockBuilder->CreateMul(
            dstAllocSizeRValue, 
            sizeAdapter->llvmConst( context, m_memberAdapter->getDesc()->getAllocSize() )
            );
          dstMemberDatasRValue = basicBlockBuilder->CreatePointerCast(
            llvmCallRealloc(
              basicBlockBuilder,
              basicBlockBuilder->CreatePointerCast(
                dstMemberDatasRValue,
                basicBlockBuilder->getInt8PtrTy()
                ),
              allocSizeRValue
              ),
            dstMemberDatasRValue->getType()
            );
          basicBlockBuilder->CreateStore( dstMemberDatasRValue, dstMemberDatasLValue );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, indexLValue, sizeAdapter->llvmConst( context, 0 ) );
          llvm::Value *srcMemberDatasLValue = basicBlockBuilder->CreateStructGEP( srcLValue, MemberDatasIndex );
          llvm::Value *srcMemberDatasRValue = basicBlockBuilder->CreateLoad( srcMemberDatasLValue );
          basicBlockBuilder->CreateBr( copyCheckBB );
          
          basicBlockBuilder->SetInsertPoint( copyCheckBB );
          indexRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, indexLValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpULT( indexRValue, srcSizeRValue ),
            copyNextBB,
            copyDoneBB
            );
          
          basicBlockBuilder->SetInsertPoint( copyNextBB );
          llvm::Value *srcMemberLValue = basicBlockBuilder->CreateGEP( srcMemberDatasRValue, indexRValue );
          llvm::Value *srcMemberRValue = m_memberAdapter->llvmLValueToRValue( basicBlockBuilder, srcMemberLValue );
          dstMemberLValue = basicBlockBuilder->CreateGEP( dstMemberDatasRValue, indexRValue );
          m_memberAdapter->llvmInit( basicBlockBuilder, dstMemberLValue );
          m_memberAdapter->llvmDefaultAssign( basicBlockBuilder, dstMemberLValue, srcMemberRValue );
          sizeAdapter->llvmDefaultAssign(
            basicBlockBuilder,
            indexLValue,
            basicBlockBuilder->CreateAdd(
              indexRValue,
              sizeAdapter->llvmConst( context, 1 )
              )
            );
          basicBlockBuilder->CreateBr( copyCheckBB );
          
          basicBlockBuilder->SetInsertPoint( copyDoneBB );
          basicBlockBuilder->CreateBr( doneBB );
          
          basicBlockBuilder->SetInsertPoint( doneBB );
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

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *resizeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "resizeBB" );
          llvm::BasicBlock *disposeCheckBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "disposeCheckBB" );
          llvm::BasicBlock *disposeNextBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "disposeNextBB" );
          llvm::BasicBlock *disposeDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "disposeDoneBB" );
          llvm::BasicBlock *zeroBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "zeroBB" );
          llvm::BasicBlock *freeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "freeBB" );
          llvm::BasicBlock *zeroDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "zeroDoneBB" );
          llvm::BasicBlock *nonZeroBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "nonZeroBB" );
          llvm::BasicBlock *expandBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "expandBB" );
          llvm::BasicBlock *expandDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "expandDoneBB" );
          llvm::BasicBlock *initCheckBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "initCheckBB" );
          llvm::BasicBlock *initNextBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "initNextBB" );
          llvm::BasicBlock *initDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "initDoneBB" );
          llvm::BasicBlock *resizeDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "resizeDoneBB" );
          llvm::BasicBlock *doneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "doneBB" );

          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *sizeLValue = basicBlockBuilder->CreateStructGEP( selfLValue, SizeIndex );
          llvm::Value *oldSizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, sizeLValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpNE( newSizeRValue, oldSizeRValue ),
            resizeBB,
            doneBB
            );

          basicBlockBuilder->SetInsertPoint( resizeBB );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, sizeLValue, newSizeRValue );
          llvm::Value *allocSizeLValue = basicBlockBuilder->CreateStructGEP( selfLValue, AllocSizeIndex );
          llvm::Value *memberDatasLValue = basicBlockBuilder->CreateStructGEP( selfLValue, MemberDatasIndex, "memberDatasLValue" );
          llvm::Value *memberDatasRValue = basicBlockBuilder->CreateLoad( memberDatasLValue, "memberDatasRValue" );
          llvm::Value *indexLValue = sizeAdapter->llvmAlloca( basicBlockBuilder, "indexLValue" );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, indexLValue, newSizeRValue );
          basicBlockBuilder->CreateBr( disposeCheckBB );
          
          basicBlockBuilder->SetInsertPoint( disposeCheckBB );
          llvm::Value *indexRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, indexLValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpULT( indexRValue, oldSizeRValue ),
            disposeNextBB,
            disposeDoneBB
            );
          
          basicBlockBuilder->SetInsertPoint( disposeNextBB );
          llvm::Value *memberLValue = basicBlockBuilder->CreateGEP( memberDatasRValue, indexRValue );
          m_memberAdapter->llvmDispose( basicBlockBuilder, memberLValue );
          sizeAdapter->llvmDefaultAssign(
            basicBlockBuilder,
            indexLValue,
            basicBlockBuilder->CreateAdd(
              indexRValue,
              sizeAdapter->llvmConst( context, 1 )
              )
            );
          basicBlockBuilder->CreateBr( disposeCheckBB );
          
          basicBlockBuilder->SetInsertPoint( disposeDoneBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpEQ( newSizeRValue, sizeAdapter->llvmConst( context, 0 ) ),
            zeroBB,
            nonZeroBB
            );
            
          basicBlockBuilder->SetInsertPoint( zeroBB );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, allocSizeLValue, sizeAdapter->llvmConst( context, 0 ) );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateIsNotNull( memberDatasRValue ),
            freeBB,
            doneBB
            );
           
          basicBlockBuilder->SetInsertPoint( freeBB );
          llvmCallFree(
            basicBlockBuilder,
            basicBlockBuilder->CreatePointerCast(
              memberDatasRValue,
              basicBlockBuilder->getInt8PtrTy()
              )
            );
          basicBlockBuilder->CreateStore(
            llvm::ConstantPointerNull::get( static_cast<llvm::PointerType const *>( memberDatasRValue->getType() ) ),
            memberDatasLValue
            );
          basicBlockBuilder->CreateBr( zeroDoneBB );
           
          basicBlockBuilder->SetInsertPoint( zeroDoneBB );
          basicBlockBuilder->CreateBr( resizeDoneBB );
           
          basicBlockBuilder->SetInsertPoint( nonZeroBB );
          llvm::Value *oldAllocSizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, allocSizeLValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpUGT( newSizeRValue, oldAllocSizeRValue ),
            expandBB,
            expandDoneBB
            );
          
          basicBlockBuilder->SetInsertPoint( expandBB );
          llvm::Value *newAllocSizeRValue = llvmCallMax(
            basicBlockBuilder,
            llvmCallNextPowTwoMinusOne(
              basicBlockBuilder,
              newSizeRValue
              ),
            sizeAdapter->llvmConst( context, 15 )
            );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, allocSizeLValue, newAllocSizeRValue );
          llvm::Value *totalSizeRValue = basicBlockBuilder->CreateMul(
            newAllocSizeRValue, 
            sizeAdapter->llvmConst( context, m_memberAdapter->getDesc()->getAllocSize() )
            );
          llvm::Value *expandedMemberDatasRValue = basicBlockBuilder->CreatePointerCast(
            llvmCallRealloc(
              basicBlockBuilder,
              basicBlockBuilder->CreatePointerCast(
                memberDatasRValue,
                basicBlockBuilder->getInt8PtrTy()
                ),
              totalSizeRValue
              ),
            memberDatasRValue->getType()
            );
          basicBlockBuilder->CreateStore( expandedMemberDatasRValue, memberDatasLValue );
          basicBlockBuilder->CreateBr( expandDoneBB );
          
          basicBlockBuilder->SetInsertPoint( expandDoneBB );
          llvm::PHINode *expandMemberDatasRValue = basicBlockBuilder->CreatePHI( memberDatasRValue->getType(), "expandMemberDatasRValue" );
          expandMemberDatasRValue->addIncoming( expandedMemberDatasRValue, expandBB );
          expandMemberDatasRValue->addIncoming( memberDatasRValue, nonZeroBB );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, indexLValue, oldSizeRValue );
          basicBlockBuilder->CreateBr( initCheckBB );
          
          basicBlockBuilder->SetInsertPoint( initCheckBB );
          indexRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, indexLValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpULT( indexRValue, newSizeRValue ),
            initNextBB,
            initDoneBB
            );
          
          basicBlockBuilder->SetInsertPoint( initNextBB );
          memberLValue = basicBlockBuilder->CreateGEP( expandMemberDatasRValue, indexRValue );
          m_memberAdapter->llvmInit( basicBlockBuilder, memberLValue );
          sizeAdapter->llvmDefaultAssign(
            basicBlockBuilder,
            indexLValue,
            basicBlockBuilder->CreateAdd(
              indexRValue,
              sizeAdapter->llvmConst( context, 1 )
              )
            );
          basicBlockBuilder->CreateBr( initCheckBB );
          
          basicBlockBuilder->SetInsertPoint( initDoneBB );
          basicBlockBuilder->CreateBr( resizeDoneBB );
          
          basicBlockBuilder->SetInsertPoint( resizeDoneBB );
          basicBlockBuilder->CreateBr( doneBB );
          
          basicBlockBuilder->SetInsertPoint( doneBB );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ParamVector params;
        params.push_back( FunctionParam( "array", this, CG::USAGE_RVALUE ) );
        params.push_back( FunctionParam( "index", sizeAdapter, CG::USAGE_RVALUE ) );
        if ( guarded )
          params.push_back( FunctionParam( "errorDesc", constStringAdapter, CG::USAGE_RVALUE ) );
        InternalFunctionBuilder functionBuilder(
          moduleBuilder,
          m_memberAdapter, "__"+getCodeName()+"__ConstIndex",
          params,
          FunctionBuilder::DirectlyReturnRValue
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *arrayRValue = functionBuilder[0];
          llvm::Value *indexRValue = functionBuilder[1];
          llvm::Value *errorDescRValue;
          if ( guarded )
            errorDescRValue = functionBuilder[2];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *inRangeBB;
          llvm::BasicBlock *outOfRangeBB;
          if ( guarded )
          {
            inRangeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "inRange" );
            outOfRangeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "outOfRange" );
          }

          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *sizeRValue;
          if ( guarded )
          {
            sizeRValue = llvmCallSize( basicBlockBuilder, arrayRValue );
            llvm::Value *inRangeCond = basicBlockBuilder->CreateICmpULT( indexRValue, sizeRValue );
            basicBlockBuilder->CreateCondBr( inRangeCond, inRangeBB, outOfRangeBB );
            
            basicBlockBuilder->SetInsertPoint( inRangeBB );
          }
          basicBlockBuilder->CreateRet( llvmConstIndexOp_NoCheck( basicBlockBuilder, arrayRValue, indexRValue ) );
          
          if ( guarded )
          {
            basicBlockBuilder->SetInsertPoint( outOfRangeBB );
            llvmThrowOutOfRangeException(
              basicBlockBuilder,
              constStringAdapter->llvmConst( basicBlockBuilder, "index" ),
              indexRValue,
              sizeRValue,
              errorDescRValue
              );
            llvm::Value *defaultRValue = m_memberAdapter->llvmDefaultRValue( basicBlockBuilder );
            basicBlockBuilder->CreateRet( defaultRValue );
          }
        }
      }

      {
        ParamVector params;
        params.push_back( FunctionParam( "array", this, CG::USAGE_LVALUE ) );
        params.push_back( FunctionParam( "index", sizeAdapter, CG::USAGE_RVALUE ) );
        if ( guarded )
          params.push_back( FunctionParam( "errorDesc", constStringAdapter, CG::USAGE_RVALUE ) );
        InternalFunctionBuilder functionBuilder(
          moduleBuilder,
          m_memberAdapter, "__"+getCodeName()+"__NonConstIndex",
          params,
          FunctionBuilder::DirectlyReturnLValue
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *arrayLValue = functionBuilder[0];
          llvm::Value *indexRValue = functionBuilder[1];
          llvm::Value *errorDescRValue;
          if ( guarded )
            errorDescRValue = functionBuilder[2];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *inRangeBB;
          llvm::BasicBlock *outOfRangeBB;
          if ( guarded )
          {
            inRangeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "inRange" );
            outOfRangeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "outOfRange" );
          }

          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *sizeRValue;
          if ( guarded )
          {
            llvm::Value *arrayRValue = llvmLValueToRValue( basicBlockBuilder, arrayLValue );
            sizeRValue = llvmCallSize( basicBlockBuilder, arrayRValue );
            llvm::Value *inRangeCond = basicBlockBuilder->CreateICmpULT( indexRValue, sizeRValue );
            basicBlockBuilder->CreateCondBr( inRangeCond, inRangeBB, outOfRangeBB );
            
            basicBlockBuilder->SetInsertPoint( inRangeBB );
          }
          basicBlockBuilder->CreateRet( llvmNonConstIndexOp_NoCheck( basicBlockBuilder, arrayLValue, indexRValue ) );
          
          if ( guarded )
          {
            basicBlockBuilder->SetInsertPoint( outOfRangeBB );
            llvmThrowOutOfRangeException(
              basicBlockBuilder,
              constStringAdapter->llvmConst( basicBlockBuilder, "index" ),
              indexRValue,
              sizeRValue,
              errorDescRValue
              );
            llvm::Constant *defaultLValue = m_memberAdapter->llvmDefaultLValue( basicBlockBuilder );
            basicBlockBuilder->CreateRet( defaultLValue );
          }
        }
      }
      
      {
        InternalFunctionBuilder functionBuilder(
          moduleBuilder,
          m_memberAdapter, "__"+getCodeName()+"__ConstIndexNoCheck",
          "array", this, CG::USAGE_RVALUE,
          "index", sizeAdapter, CG::USAGE_RVALUE,
          FunctionBuilder::DirectlyReturnRValue
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *arrayRValue = functionBuilder[0];
          llvm::Value *indexRValue = functionBuilder[1];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );

          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *memberLValue = llvmNonConstIndexOp_NoCheck( basicBlockBuilder, arrayRValue, indexRValue );
          llvm::Value *memberRValue = m_memberAdapter->llvmLValueToRValue( basicBlockBuilder, memberLValue );
          basicBlockBuilder->CreateRet( memberRValue );
        }
      }

      {
        InternalFunctionBuilder functionBuilder(
          moduleBuilder,
          m_memberAdapter, "__"+getCodeName()+"__NonConstIndexNoCheck",
          "array", this, CG::USAGE_LVALUE,
          "index", sizeAdapter, CG::USAGE_RVALUE,
          FunctionBuilder::DirectlyReturnLValue
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *arrayLValue = functionBuilder[0];
          llvm::Value *indexRValue = functionBuilder[1];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );

          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *memberDatasLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, MemberDatasIndex );
          llvm::Value *memberDatasRValue = basicBlockBuilder->CreateLoad( memberDatasLValue );
          llvm::Value *memberLValue = basicBlockBuilder->CreateGEP( memberDatasRValue, indexRValue );
          basicBlockBuilder->CreateRet( memberLValue );
        }
      }

      {
        ConstructorBuilder functionBuilder( moduleBuilder, booleanAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *booleanLValue = functionBuilder[0];
          llvm::Value *arrayRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *sizeRValue = llvmCallSize( basicBlockBuilder, arrayRValue );
          llvm::Value *booleanRValue = basicBlockBuilder->CreateICmpNE( sizeRValue, llvm::ConstantInt::get( sizeRValue->getType(), 0 ) );
          booleanAdapter->llvmAssign( basicBlockBuilder, booleanLValue, booleanRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, stringAdapter, this );
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
      
      llvm::Function *assignAddFunction;
      {
        std::vector< llvm::Type const * > argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( context->getLLVMContext() ) );
        argTypes.push_back( llvmLType( context ) );
        argTypes.push_back( llvmRType( context ) );
        llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
        llvm::Constant *func = moduleBuilder->getOrInsertFunction( "__"+getCodeName()+"__Append", funcType ); 

        AssOpBuilder functionBuilder( moduleBuilder, this, ASSIGN_OP_ADD, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsLValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          std::vector< llvm::Value * > args;
          args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
          args.push_back( lhsLValue );
          args.push_back( rhsRValue );
          basicBlockBuilder->CreateCall( func, args.begin(), args.end() );
          basicBlockBuilder->CreateRetVoid();
        }
        assignAddFunction = functionBuilder.getLLVMFunction();
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
          0,
          this, USAGE_LVALUE,
          "swap",
          "lhs", sizeAdapter, USAGE_RVALUE,
          "rhs", sizeAdapter, USAGE_RVALUE
          );
        if ( buildFunctions )
        {
          llvm::Value *thisLValue = functionBuilder[0];
          llvm::Value *lhsRValue = functionBuilder[1];
          llvm::Value *rhsRValue = functionBuilder[2];

          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          llvm::BasicBlock *unequalBB = functionBuilder.createBasicBlock( "unequal" );
          llvm::BasicBlock *doneBB = functionBuilder.createBasicBlock( "done" );

          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( entryBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpEQ( lhsRValue, rhsRValue ),
            doneBB,
            unequalBB
            );

          basicBlockBuilder->SetInsertPoint( unequalBB );
          llvm::Value *tmpLValue = m_memberAdapter->llvmAlloca( basicBlockBuilder, "tmp" );
          m_memberAdapter->llvmInit( basicBlockBuilder, tmpLValue );
          llvm::Value *lhsMemberLValue = llvmNonConstIndexOp( basicBlockBuilder, thisLValue, lhsRValue, 0 );
          llvm::Value *lhsMemberRValue = m_memberAdapter->llvmLValueToRValue( basicBlockBuilder, lhsMemberLValue );
          llvm::Value *rhsMemberLValue = llvmNonConstIndexOp( basicBlockBuilder, thisLValue, rhsRValue, 0 );
          llvm::Value *rhsMemberRValue = m_memberAdapter->llvmLValueToRValue( basicBlockBuilder, rhsMemberLValue );
          m_memberAdapter->llvmDefaultAssign( basicBlockBuilder, tmpLValue, lhsMemberRValue );
          m_memberAdapter->llvmDefaultAssign( basicBlockBuilder, lhsMemberLValue, rhsMemberRValue );
          m_memberAdapter->llvmDefaultAssign( basicBlockBuilder, rhsMemberLValue, m_memberAdapter->llvmLValueToRValue( basicBlockBuilder, tmpLValue ) );
          m_memberAdapter->llvmDispose( basicBlockBuilder, tmpLValue );
          basicBlockBuilder->CreateBr( doneBB );

          basicBlockBuilder->SetInsertPoint( doneBB );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          0,
          this, USAGE_LVALUE,
          "push",
          "element", m_memberAdapter, USAGE_RVALUE
          );
        if ( buildFunctions )
        {
          llvm::Value *thisLValue = functionBuilder[0];
          llvm::Value *memberRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *thisRValue = llvmLValueToRValue( basicBlockBuilder, thisLValue );
          llvm::Value *oldSize = llvmCallSize( basicBlockBuilder, thisRValue );
          llvm::Value *newSize = basicBlockBuilder->CreateAdd( oldSize, sizeAdapter->llvmConst( context, 1 ) );
          llvmCallResize( basicBlockBuilder, thisLValue, newSize );
          llvm::Value *newElementLValue = llvmNonConstIndexOp( basicBlockBuilder, thisLValue, oldSize, 0 );
          m_memberAdapter->llvmAssign( basicBlockBuilder, newElementLValue, memberRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          m_memberAdapter,
          this, USAGE_LVALUE,
          "pop"
          );
        if ( buildFunctions )
        {
          ReturnInfo const &returnInfo = functionBuilder.getScope().getReturnInfo();
          llvm::Value *thisLValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
        
          llvm::Value *returnLValue;
          if( !returnInfo.usesReturnLValue() )
          {
            returnLValue = m_memberAdapter->llvmAlloca( basicBlockBuilder, "result" );
            m_memberAdapter->llvmInit( basicBlockBuilder, returnLValue );
          }
          else
          {
            returnLValue = returnInfo.getReturnLValue();
          }
          llvmCallPop( basicBlockBuilder, thisLValue, returnLValue );
          
          if( returnInfo.usesReturnLValue() )
          {
            basicBlockBuilder->CreateRetVoid( );
          }
          else
          {
            llvm::Value *returnRValue = m_memberAdapter->llvmLValueToRValue( basicBlockBuilder, returnLValue );
            basicBlockBuilder->CreateRet( returnRValue );
          }
        }
      }
      
      if ( m_memberAdapter->getImpl()->isShallow() )
      {
        {
          MethodBuilder functionBuilder(
            moduleBuilder,
            sizeAdapter,
            this, USAGE_RVALUE,
            "dataSize"
            );
          if ( buildFunctions )
          {
            llvm::Value *selfRValue = functionBuilder[0];
            BasicBlockBuilder basicBlockBuilder( functionBuilder );
            basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
            llvm::Value *sizeRValue = llvmCallSize( basicBlockBuilder, selfRValue );
            llvm::Value *memberSizeRValue = sizeAdapter->llvmConst( context, m_memberAdapter->getDesc()->getAllocSize() );
            llvm::Value *dataSizeRValue = basicBlockBuilder->CreateMul( sizeRValue, memberSizeRValue );
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
            llvm::Value *selfRValue = functionBuilder[0];
            BasicBlockBuilder basicBlockBuilder( functionBuilder );
            basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
            llvm::Value *selfLValue = llvmRValueToLValue( basicBlockBuilder, selfRValue );
            llvm::Value *memberDatasLValue = basicBlockBuilder->CreateStructGEP( selfLValue, MemberDatasIndex );
            llvm::Value *memberDatasRValue = basicBlockBuilder->CreateLoad( memberDatasLValue );
            basicBlockBuilder->CreateRet(
              basicBlockBuilder->CreatePointerCast( memberDatasRValue, dataAdapter->llvmRType( context ) )
              );
          }
        }
      }
    }
    
    void *VariableArrayAdapter::llvmResolveExternalFunction( std::string const &functionName ) const
    {
      if ( functionName == "__"+getCodeName()+"__Pop" )
        return (void *)&VariableArrayAdapter::Pop;
      else if ( functionName == "__"+getCodeName()+"__Append" )
        return (void *)&VariableArrayAdapter::Append;
      else return ArrayAdapter::llvmResolveExternalFunction( functionName );
    }

    llvm::Value *VariableArrayAdapter::llvmConstIndexOp(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *arrayRValue,
      llvm::Value *indexRValue,
      CG::Location const *location
      ) const
    {
      bool const guarded = basicBlockBuilder.getModuleBuilder().getCompileOptions()->getGuarded();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      RC::ConstHandle<ConstStringAdapter> constStringAdapter = basicBlockBuilder.getManager()->getConstStringAdapter();
      ParamVector params;
      params.push_back( FunctionParam( "array", this, CG::USAGE_RVALUE ) );
      params.push_back( FunctionParam( "index", sizeAdapter, CG::USAGE_RVALUE ) );
      if ( guarded )
        params.push_back( FunctionParam( "errorDesc", constStringAdapter, CG::USAGE_RVALUE ) );
      InternalFunctionBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        m_memberAdapter, "__"+getCodeName()+"__ConstIndex",
        params,
        FunctionBuilder::DirectlyReturnRValue
        );
      std::vector<llvm::Value *> args;
      args.push_back( arrayRValue );
      args.push_back( indexRValue );
      if ( guarded )
        args.push_back( llvmLocationConstStringRValue( basicBlockBuilder, constStringAdapter, location ) );
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), args.begin(), args.end() );
    }

    llvm::Value *VariableArrayAdapter::llvmNonConstIndexOp(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *exprLValue,
      llvm::Value *indexRValue,
      CG::Location const *location
      ) const
    {
      bool const guarded = basicBlockBuilder.getModuleBuilder().getCompileOptions()->getGuarded();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      RC::ConstHandle<ConstStringAdapter> constStringAdapter = basicBlockBuilder.getManager()->getConstStringAdapter();
      ParamVector params;
      params.push_back( FunctionParam( "array", this, CG::USAGE_LVALUE ) );
      params.push_back( FunctionParam( "index", sizeAdapter, CG::USAGE_RVALUE ) );
      if ( guarded )
        params.push_back( FunctionParam( "errorDesc", constStringAdapter, CG::USAGE_RVALUE ) );
      InternalFunctionBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        m_memberAdapter, "__"+getCodeName()+"__NonConstIndex",
        params,
        FunctionBuilder::DirectlyReturnLValue
        );
      std::vector<llvm::Value *> args;
      args.push_back( exprLValue );
      args.push_back( indexRValue );
      if ( guarded )
        args.push_back( llvmLocationConstStringRValue( basicBlockBuilder, constStringAdapter, location ) );
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), args.begin(), args.end() );
    }

    llvm::Value *VariableArrayAdapter::llvmConstIndexOp_NoCheck( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayRValue, llvm::Value *indexRValue ) const
    {
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      InternalFunctionBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        m_memberAdapter, "__"+getCodeName()+"__ConstIndexNoCheck",
        "array", this, CG::USAGE_RVALUE,
        "index", sizeAdapter, CG::USAGE_RVALUE,
        FunctionBuilder::DirectlyReturnRValue
        );
      return basicBlockBuilder->CreateCall2( functionBuilder.getLLVMFunction(), arrayRValue, indexRValue );
    }

    llvm::Value *VariableArrayAdapter::llvmNonConstIndexOp_NoCheck( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *exprLValue, llvm::Value *indexRValue ) const
    {
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      InternalFunctionBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        m_memberAdapter, "__"+getCodeName()+"__NonConstIndexNoCheck",
        "array", this, CG::USAGE_LVALUE,
        "index", sizeAdapter, CG::USAGE_RVALUE,
        FunctionBuilder::DirectlyReturnLValue
        );
      return basicBlockBuilder->CreateCall2( functionBuilder.getLLVMFunction(), exprLValue, indexRValue );
    }
    
    void VariableArrayAdapter::Append( VariableArrayAdapter const *inst, void *dstLValue, void const *srcRValue )
    {
      inst->m_variableArrayDesc->append( dstLValue, srcRValue );
    }
    
    void VariableArrayAdapter::Pop( VariableArrayAdapter const *inst, void *dst, void *result )
    {
      inst->m_variableArrayDesc->pop( dst, result );
    }

    void VariableArrayAdapter::llvmCallResize( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayLValue, llvm::Value *newSizeRValue ) const
    {
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      MethodBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        0,
        this, USAGE_LVALUE,
        "resize",
        "newSize", sizeAdapter, USAGE_RVALUE
        );
      basicBlockBuilder->CreateCall2( functionBuilder.getLLVMFunction(), arrayLValue, newSizeRValue );
    }

    void VariableArrayAdapter::llvmCallPop( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayLValue, llvm::Value *memberLValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      
      std::vector< llvm::Type const * > argTypes;
      argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
      argTypes.push_back( llvmLType( context ) );
      argTypes.push_back( m_memberAdapter->llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
      llvm::AttributeWithIndex AWI[4];
      AWI[0] = llvm::AttributeWithIndex::get( 1, llvm::Attribute::NoCapture );
      AWI[1] = llvm::AttributeWithIndex::get( 2, llvm::Attribute::NoCapture );
      AWI[2] = llvm::AttributeWithIndex::get( 3, llvm::Attribute::NoCapture );
      AWI[3] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 4 );
      
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Pop", funcType, attrListPtr ) ); 

      std::vector< llvm::Value * > args;
      args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
      args.push_back( arrayLValue );
      args.push_back( memberLValue );
      basicBlockBuilder->CreateCall( func, args.begin(), args.end() );
    }
    
    llvm::Value *VariableArrayAdapter::llvmCallSize( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayRValue ) const
    {
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      MethodBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        sizeAdapter,
        this, USAGE_RVALUE,
        "size"
        );
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), arrayRValue );
    }

    void VariableArrayAdapter::llvmDisposeImpl( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *selfLValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();

      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );

      std::string name = "__" + getCodeName() + "__Dispose";
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getFunction( name ) );
      if ( !func )
      {
        ModuleBuilder &mb = basicBlockBuilder.getModuleBuilder();
        
        func = llvm::cast<llvm::Function>( mb->getOrInsertFunction( name, funcType, attrListPtr ) ); 
        func->setLinkage( llvm::GlobalValue::PrivateLinkage );
        
        FunctionBuilder fb( mb, funcType, func );
        llvm::Argument *selfLValue = fb[0];
        selfLValue->setName( "selfLValue" );
        selfLValue->addAttr( llvm::Attribute::NoCapture );
        selfLValue->addAttr( llvm::Attribute::NoAlias );
        
        BasicBlockBuilder bbb( fb );
        llvm::BasicBlock *entryBB = fb.createBasicBlock( "entry" );
        llvm::BasicBlock *checkBB = fb.createBasicBlock( "membersCheck" );
        llvm::BasicBlock *nextBB = fb.createBasicBlock( "membersNext" );
        llvm::BasicBlock *disposeDoneBB = fb.createBasicBlock( "membersDone" );
        llvm::BasicBlock *freeBB = fb.createBasicBlock( "free" );
        llvm::BasicBlock *doneBB = fb.createBasicBlock( "done" );
        
        bbb->SetInsertPoint( entryBB );
        llvm::Value *sizeLValue = bbb->CreateStructGEP( selfLValue, SizeIndex );
        llvm::Value *sizeRValue = sizeAdapter->llvmLValueToRValue( bbb, sizeLValue );
        llvm::Value *memberDatasLValue = bbb->CreateStructGEP( selfLValue, MemberDatasIndex );
        llvm::Value *memberDatasRValue = bbb->CreateLoad( memberDatasLValue );
        llvm::Value *indexLValue = sizeAdapter->llvmAlloca( bbb, "index" );
        sizeAdapter->llvmInit( bbb, indexLValue );
        bbb->CreateBr( checkBB );
        
        bbb->SetInsertPoint( checkBB );
        llvm::Value *indexRValue = sizeAdapter->llvmLValueToRValue( bbb, indexLValue );
        bbb->CreateCondBr(
          bbb->CreateICmpULT( indexRValue, sizeRValue ),
          nextBB,
          disposeDoneBB
          );
          
        bbb->SetInsertPoint( nextBB );
        llvm::Value *memberLValue = bbb->CreateGEP( memberDatasRValue, indexRValue );
        m_memberAdapter->llvmDispose( bbb, memberLValue );
        llvm::Value *nextIndexRValue = bbb->CreateAdd( indexRValue, sizeAdapter->llvmConst( context, 1 ) );
        sizeAdapter->llvmDefaultAssign( bbb, indexLValue, nextIndexRValue );
        bbb->CreateBr( checkBB );
          
        bbb->SetInsertPoint( disposeDoneBB );
        bbb->CreateCondBr(
          bbb->CreateIsNotNull( memberDatasRValue ),
          freeBB,
          doneBB
          );
        
        bbb->SetInsertPoint( freeBB );
        llvmCallFree( bbb, memberDatasRValue );
        bbb->CreateBr( doneBB );

        bbb->SetInsertPoint( doneBB );
        bbb->CreateRetVoid();
      }
      
      basicBlockBuilder->CreateCall( func, selfLValue );
    }

    void VariableArrayAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      InternalFunctionBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        0, "__"+getCodeName()+"__DefaultAssign",
        "dst", this, CG::USAGE_LVALUE,
        "src", this, CG::USAGE_RVALUE,
        0
        );
      basicBlockBuilder->CreateCall2( functionBuilder.getLLVMFunction(), dstLValue, srcRValue );
    }
    
    llvm::Constant *VariableArrayAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      std::vector<llvm::Constant *> memberDefaultRValues;
      memberDefaultRValues.push_back( sizeAdapter->llvmConst( context, 0 ) );
      memberDefaultRValues.push_back( sizeAdapter->llvmConst( context, 0 ) );
      memberDefaultRValues.push_back( llvm::ConstantPointerNull::get( static_cast<llvm::PointerType const *>( m_memberAdapter->llvmLType( context ) ) ) );
      return llvm::ConstantStruct::get( context->getLLVMContext(), memberDefaultRValues, true );
    }
      
    llvm::Constant *VariableArrayAdapter::llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmDefaultLValue( basicBlockBuilder );
    }

    llvm::Constant *VariableArrayAdapter::llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const
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
    
    void VariableArrayAdapter::llvmInit( BasicBlockBuilder &basicBlockBuilder, llvm::Value *selfLValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();

      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );

      std::string name = "__" + getCodeName() + "__Init";
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getFunction( name ) );
      if ( !func )
      {
        ModuleBuilder &mb = basicBlockBuilder.getModuleBuilder();
        
        func = llvm::cast<llvm::Function>( mb->getOrInsertFunction( name, funcType, attrListPtr ) ); 
        func->setLinkage( llvm::GlobalValue::PrivateLinkage );
        
        FunctionBuilder fb( mb, funcType, func );
        llvm::Argument *selfLValue = fb[0];
        selfLValue->setName( "selfLValue" );
        selfLValue->addAttr( llvm::Attribute::NoCapture );
        selfLValue->addAttr( llvm::Attribute::NoAlias );
        
        BasicBlockBuilder bbb( fb );
        llvm::BasicBlock *entryBB = fb.createBasicBlock( "entry" );
        bbb->SetInsertPoint( entryBB );
        
        llvm::Value *allocSizeLValue = bbb->CreateStructGEP( selfLValue, AllocSizeIndex );
        sizeAdapter->llvmDefaultAssign( bbb, allocSizeLValue, sizeAdapter->llvmConst( context, 0 ) );
        llvm::Value *sizeLValue = bbb->CreateStructGEP( selfLValue, SizeIndex );
        sizeAdapter->llvmDefaultAssign( bbb, sizeLValue, sizeAdapter->llvmConst( context, 0 ) );
        llvm::Value *memberDatasLValue = bbb->CreateStructGEP( selfLValue, MemberDatasIndex );
        bbb->CreateStore( llvm::ConstantPointerNull::get( static_cast<llvm::PointerType const *>( m_memberAdapter->llvmLType( context ) ) ), memberDatasLValue );
        bbb->CreateRetVoid();
      }
      
      basicBlockBuilder->CreateCall( func, selfLValue );
    }
  };
};
