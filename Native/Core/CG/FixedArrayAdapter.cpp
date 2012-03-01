/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "FixedArrayAdapter.h"
#include "BooleanAdapter.h"
#include "IntegerAdapter.h"
#include "SizeAdapter.h"
#include "ConstStringAdapter.h"
#include "StringAdapter.h"
#include "OpaqueAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "InternalFunctionBuilder.h"
#include "BasicBlockBuilder.h"
#include "MethodBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/CG/CompileOptions.h>
#include <Fabric/Core/RT/FixedArrayDesc.h>

#include <llvm/Module.h>
#include <llvm/Function.h>

namespace Fabric
{
  namespace CG
  {
    FixedArrayAdapter::FixedArrayAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::FixedArrayDesc> const &fixedArrayDesc )
      : ArrayAdapter( manager, fixedArrayDesc, FL_PASS_BY_REFERENCE )
      , m_length( fixedArrayDesc->getNumMembers() )
      , m_fixedArrayDesc( fixedArrayDesc )
      , m_memberAdapter( manager->getAdapter( fixedArrayDesc->getMemberDesc() ) )
    {
    }
    
    llvm::Type const *FixedArrayAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      return llvm::ArrayType::get( m_memberAdapter->llvmRawType( context ), m_length );
    }
    
    void FixedArrayAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
        
      RC::Handle<Context> context = moduleBuilder.getContext();
      
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

      moduleBuilder->addTypeName( getCodeName(), llvmRawType( context ) );
      
      static const bool buildFunctions = true;
      bool const guarded = moduleBuilder.getCompileOptions()->getGuarded();
      
      {
        InternalFunctionBuilder functionBuilder( moduleBuilder, 0, "__"+getCodeName()+"__DefaultAssign", "dst", this, CG::USAGE_LVALUE, "src", this, CG::USAGE_RVALUE, 0 );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *dstLValue = functionBuilder[0];
          llvm::Value *srcRValue = functionBuilder[1];
        
          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *checkBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "check" );
          llvm::BasicBlock *notDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "notDone" );
          llvm::BasicBlock *doneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "done" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *lengthRValue = sizeAdapter->llvmConst( context, m_length );
          llvm::Value *indexLValue = sizeAdapter->llvmAlloca( basicBlockBuilder, "index" );
          basicBlockBuilder->CreateStore( sizeAdapter->llvmConst( context, 0 ), indexLValue );
          basicBlockBuilder->CreateBr( checkBB );
          
          basicBlockBuilder->SetInsertPoint( checkBB );
          llvm::Value *indexRValue = basicBlockBuilder->CreateLoad( indexLValue );
          basicBlockBuilder->CreateCondBr( basicBlockBuilder->CreateICmpULT( indexRValue, lengthRValue ), notDoneBB, doneBB );
          
          basicBlockBuilder->SetInsertPoint( notDoneBB );
          indexRValue = basicBlockBuilder->CreateLoad( indexLValue );
          llvm::Value *dstMemberData = basicBlockBuilder->CreateConstGEP2_32( dstLValue, 0, 0 );
          llvm::Value *dstMemberLValue = basicBlockBuilder->CreateGEP( dstMemberData, indexRValue );
          llvm::Value *srcMemberData = basicBlockBuilder->CreateConstGEP2_32( srcRValue, 0, 0 );
          llvm::Value *srcMemberLValue = basicBlockBuilder->CreateGEP( srcMemberData, indexRValue );
          llvm::Value *srcMemberRValue = m_memberAdapter->llvmLValueToRValue( basicBlockBuilder, srcMemberLValue );
          m_memberAdapter->llvmAssign( basicBlockBuilder, dstMemberLValue, srcMemberRValue );
          llvm::Value *newIndexRValue = basicBlockBuilder->CreateAdd( indexRValue, sizeAdapter->llvmConst( context, 1 ) );
          basicBlockBuilder->CreateStore( newIndexRValue, indexLValue );
          basicBlockBuilder->CreateBr( checkBB );
          
          basicBlockBuilder->SetInsertPoint( doneBB );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        InternalFunctionBuilder functionBuilder( moduleBuilder, 0, "__"+getCodeName()+"__Store", "dst", this, CG::USAGE_LVALUE, "src", this, CG::USAGE_RVALUE, 0 );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *dstLValue = functionBuilder[0];
          llvm::Value *srcRValue = functionBuilder[1];
        
          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *checkBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "check" );
          llvm::BasicBlock *notDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "notDone" );
          llvm::BasicBlock *doneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "done" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *lengthRValue = sizeAdapter->llvmConst( context, m_length );
          llvm::Value *indexLValue = sizeAdapter->llvmAlloca( basicBlockBuilder, "index" );
          basicBlockBuilder->CreateStore( sizeAdapter->llvmConst( context, 0 ), indexLValue );
          basicBlockBuilder->CreateBr( checkBB );
          
          basicBlockBuilder->SetInsertPoint( checkBB );
          llvm::Value *indexRValue = basicBlockBuilder->CreateLoad( indexLValue );
          basicBlockBuilder->CreateCondBr( basicBlockBuilder->CreateICmpULT( indexRValue, lengthRValue ), notDoneBB, doneBB );
          
          basicBlockBuilder->SetInsertPoint( notDoneBB );
          indexRValue = basicBlockBuilder->CreateLoad( indexLValue );
          llvm::Value *dstMemberData = basicBlockBuilder->CreateConstGEP2_32( dstLValue, 0, 0 );
          llvm::Value *dstMemberLValue = basicBlockBuilder->CreateGEP( dstMemberData, indexRValue );
          llvm::Value *srcMemberData = basicBlockBuilder->CreateConstGEP2_32( srcRValue, 0, 0 );
          llvm::Value *srcMemberLValue = basicBlockBuilder->CreateGEP( srcMemberData, indexRValue );
          llvm::Value *srcMemberRValue = m_memberAdapter->llvmLValueToRValue( basicBlockBuilder, srcMemberLValue );
          m_memberAdapter->llvmStore( basicBlockBuilder, dstMemberLValue, srcMemberRValue );
          llvm::Value *newIndexRValue = basicBlockBuilder->CreateAdd( indexRValue, sizeAdapter->llvmConst( context, 1 ) );
          basicBlockBuilder->CreateStore( newIndexRValue, indexLValue );
          basicBlockBuilder->CreateBr( checkBB );
          
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
        InternalFunctionBuilder functionBuilder( moduleBuilder, m_memberAdapter, "__"+getCodeName()+"__ConstIndex", params, FunctionBuilder::DirectlyReturnRValue );
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
          llvm::Value *lengthRValue;
          if ( guarded )
          {
            lengthRValue = sizeAdapter->llvmConst( context, m_length );
            llvm::Value *inRangeCond = basicBlockBuilder->CreateICmpULT( indexRValue, lengthRValue );
            basicBlockBuilder->CreateCondBr( inRangeCond, inRangeBB, outOfRangeBB );

            basicBlockBuilder->SetInsertPoint( inRangeBB );
          }
          
          llvm::Value *arrayLValue = llvmRValueToLValue( basicBlockBuilder, arrayRValue );
          llvm::Value *arrayData = basicBlockBuilder->CreateConstGEP2_32( arrayLValue, 0, 0 );
          llvm::Value *memberLValue = basicBlockBuilder->CreateGEP( arrayData, indexRValue );
          llvm::Value *memberRValue = m_memberAdapter->llvmLValueToRValue( basicBlockBuilder, memberLValue );
          basicBlockBuilder->CreateRet( memberRValue );
          
          if ( guarded )
          {
            basicBlockBuilder->SetInsertPoint( outOfRangeBB );
            llvmThrowOutOfRangeException(
              basicBlockBuilder,
              constStringAdapter->llvmConst( basicBlockBuilder, "index" ),
              indexRValue,
              lengthRValue,
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
        InternalFunctionBuilder functionBuilder( moduleBuilder, m_memberAdapter, "__"+getCodeName()+"__NonConstIndex", params, FunctionBuilder::DirectlyReturnLValue );
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
          llvm::Value *lengthRValue;
          if ( guarded )
          {
            lengthRValue = sizeAdapter->llvmConst( context, m_length );
            llvm::Value *inRangeCond = basicBlockBuilder->CreateICmpULT( indexRValue, lengthRValue );
            basicBlockBuilder->CreateCondBr( inRangeCond, inRangeBB, outOfRangeBB );
            
            basicBlockBuilder->SetInsertPoint( inRangeBB );
          }
          llvm::Value *arrayData = basicBlockBuilder->CreateConstGEP2_32( arrayLValue, 0, 0 );
          llvm::Value *memberLValue = basicBlockBuilder->CreateGEP( arrayData, indexRValue );
          basicBlockBuilder->CreateRet( memberLValue );
          
          if ( guarded )
          {
            basicBlockBuilder->SetInsertPoint( outOfRangeBB );
            llvmThrowOutOfRangeException(
              basicBlockBuilder,
              constStringAdapter->llvmConst( basicBlockBuilder, "index" ),
              indexRValue,
              lengthRValue,
              errorDescRValue
              );
            llvm::Constant *defaultLValue = m_memberAdapter->llvmDefaultLValue( basicBlockBuilder );
            basicBlockBuilder->CreateRet( defaultLValue );
          }
        }
      }

      {
        ConstructorBuilder functionBuilder( moduleBuilder, booleanAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *booleanLValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *sizeRValue = llvm::ConstantInt::get( sizeAdapter->llvmRType( context ), m_length, false );
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
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *sizeRValue = llvm::ConstantInt::get( sizeAdapter->llvmRType( context ), m_length, false );
          basicBlockBuilder->CreateRet( sizeRValue );
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
            BasicBlockBuilder basicBlockBuilder( functionBuilder );
            basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
            llvm::Value *sizeRValue = sizeAdapter->llvmConst( context, m_length );
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
            llvm::Value *thisRValue = functionBuilder[0];
            BasicBlockBuilder basicBlockBuilder( functionBuilder );
            basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
            basicBlockBuilder->CreateRet( basicBlockBuilder->CreatePointerCast( thisRValue, dataAdapter->llvmRType( context ) ) );
          }
        }
      }
    }
    
    llvm::Constant *FixedArrayAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      std::vector<llvm::Constant *> elementDefaultRValues;
      elementDefaultRValues.resize( m_length, m_memberAdapter->llvmDefaultValue( basicBlockBuilder ) );
      return llvm::ConstantArray::get( (llvm::ArrayType const *)llvmRawType( basicBlockBuilder.getContext() ), elementDefaultRValues );
    }
    
    llvm::Constant *FixedArrayAdapter::llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmDefaultLValue( basicBlockBuilder );
    }

    llvm::Constant *FixedArrayAdapter::llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const
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
    
    llvm::Value *FixedArrayAdapter::llvmConstIndexOp(
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
      InternalFunctionBuilder functionBuilder( basicBlockBuilder.getModuleBuilder(), m_memberAdapter, "__"+getCodeName()+"__ConstIndex", params, FunctionBuilder::DirectlyReturnRValue );
      std::vector<llvm::Value *> args;
      args.push_back( arrayRValue );
      args.push_back( indexRValue );
      if ( guarded )
        args.push_back( llvmLocationConstStringRValue( basicBlockBuilder, constStringAdapter, location ) );
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), args.begin(), args.end() );
    }


    llvm::Value *FixedArrayAdapter::llvmNonConstIndexOp(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *arrayLValue,
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
      InternalFunctionBuilder functionBuilder( basicBlockBuilder.getModuleBuilder(), m_memberAdapter, "__"+getCodeName()+"__NonConstIndex", params, FunctionBuilder::DirectlyReturnLValue );
      std::vector<llvm::Value *> args;
      args.push_back( arrayLValue );
      args.push_back( indexRValue );
      if ( guarded )
        args.push_back( llvmLocationConstStringRValue( basicBlockBuilder, constStringAdapter, location ) );
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), args.begin(), args.end() );
    }
    
    void FixedArrayAdapter::llvmDefaultAssign( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      InternalFunctionBuilder functionBuilder( basicBlockBuilder.getModuleBuilder(), 0, "__"+getCodeName()+"__DefaultAssign", "dst", this, CG::USAGE_LVALUE, "src", this, CG::USAGE_RVALUE, 0 );
      basicBlockBuilder->CreateCall2( functionBuilder.getLLVMFunction(), dstLValue, srcRValue );
    }
    
    void FixedArrayAdapter::llvmStore( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      InternalFunctionBuilder functionBuilder( basicBlockBuilder.getModuleBuilder(), 0, "__"+getCodeName()+"__Store", "dst", this, CG::USAGE_LVALUE, "src", this, CG::USAGE_RVALUE, 0 );
      basicBlockBuilder->CreateCall2( functionBuilder.getLLVMFunction(), dstLValue, srcRValue );
    }

    void FixedArrayAdapter::llvmDisposeImpl( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();

      llvm::BasicBlock *checkBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "faDisposeCheck" );
      llvm::BasicBlock *notDoneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "faDisposeNotDone" );
      llvm::BasicBlock *doneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "faDisposeDone" );

      llvm::Value *lengthRValue = sizeAdapter->llvmConst( basicBlockBuilder.getContext(), m_length );
      llvm::Value *indexLValue = sizeAdapter->llvmAlloca( basicBlockBuilder, "index" );
      basicBlockBuilder->CreateStore( sizeAdapter->llvmConst( basicBlockBuilder.getContext(), 0 ), indexLValue );
      basicBlockBuilder->CreateBr( checkBB );

      basicBlockBuilder->SetInsertPoint( checkBB );
      llvm::Value *indexRValue = basicBlockBuilder->CreateLoad( indexLValue );
      basicBlockBuilder->CreateCondBr( basicBlockBuilder->CreateICmpULT( indexRValue, lengthRValue ), notDoneBB, doneBB );

      basicBlockBuilder->SetInsertPoint( notDoneBB );
      indexRValue = basicBlockBuilder->CreateLoad( indexLValue );
      llvm::Value *memberLValue = basicBlockBuilder->CreateGEP(
        basicBlockBuilder->CreateConstGEP2_32( lValue, 0, 0 ),
        indexRValue
        );
      m_memberAdapter->llvmDispose( basicBlockBuilder, memberLValue );
      llvm::Value *newIndexRValue = basicBlockBuilder->CreateAdd( indexRValue, sizeAdapter->llvmConst( basicBlockBuilder.getContext(), 1 ) );
      basicBlockBuilder->CreateStore( newIndexRValue, indexLValue );
      basicBlockBuilder->CreateBr( checkBB );

      basicBlockBuilder->SetInsertPoint( doneBB );
    }
  };
};
