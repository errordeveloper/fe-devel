/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "SlicedArrayAdapter.h"
#include "VariableArrayAdapter.h"
#include "BooleanAdapter.h"
#include "SizeAdapter.h"
#include "ConstStringAdapter.h"
#include "StringAdapter.h"
#include "OpaqueAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "BasicBlockBuilder.h"
#include "InternalFunctionBuilder.h"
#include <Fabric/Core/CG/Mangling.h>
#include "CompileOptions.h"

#include <Fabric/Core/RT/SlicedArrayDesc.h>
#include <Fabric/Core/RT/SlicedArrayImpl.h>
#include <Fabric/Core/RT/VariableArrayImpl.h>
#include <Fabric/Core/RT/Impl.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Intrinsics.h>

namespace Fabric
{
  namespace CG
  {
    SlicedArrayAdapter::SlicedArrayAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::SlicedArrayDesc> const &slicedArrayDesc )
      : ArrayAdapter( manager, slicedArrayDesc, FL_PASS_BY_REFERENCE )
      , m_slicedArrayDesc( slicedArrayDesc )
      , m_slicedArrayImpl( slicedArrayDesc->getImpl() )
      , m_memberAdapter( manager->getAdapter( slicedArrayDesc->getMemberDesc() ) )
      , m_variableArrayAdapter( manager->getVariableArrayOf( m_memberAdapter ) )
    {
    }
    
    llvm::Type const *SlicedArrayAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::Type const *llvmSizeTy = llvmSizeType( context );
      
      std::vector<llvm::Type const *> memberLLVMTypes;
      memberLLVMTypes.push_back( llvmSizeTy ); // offset
      memberLLVMTypes.push_back( llvmSizeTy ); // size
      memberLLVMTypes.push_back( m_variableArrayAdapter->llvmRawType( context ) ); // rcva *
      return llvm::StructType::get( context->getLLVMContext(), memberLLVMTypes, true );
    }

    void SlicedArrayAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
      
      RC::Handle<Context> context = moduleBuilder.getContext();
        
      moduleBuilder->addTypeName( getCodeName(), llvmRawType(context) );

      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<OpaqueAdapter> dataAdapter = getManager()->getDataAdapter();
      dataAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<ConstStringAdapter> constStringAdapter = getManager()->getConstStringAdapter();
      constStringAdapter->llvmCompileToModule( moduleBuilder );

      m_memberAdapter->llvmCompileToModule( moduleBuilder );
      m_variableArrayAdapter->llvmCompileToModule( moduleBuilder );
      
      static const bool buildFunctions = true;
      bool const guarded = moduleBuilder.getCompileOptions()->getGuarded();

      {
        ConstructorBuilder functionBuilder( moduleBuilder, this, sizeAdapter, ConstructorBuilder::HighCost );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *saLValue = functionBuilder[0];
          llvm::Value *sizeRValue = functionBuilder[1];
        
          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *offsetLValue = basicBlockBuilder->CreateStructGEP( saLValue, 0 );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, offsetLValue, sizeAdapter->llvmConst( context, 0 ) );
          llvm::Value *sizeLValue = basicBlockBuilder->CreateStructGEP( saLValue, 1 );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, sizeLValue, sizeRValue );
          llvm::Value *vaLValue = basicBlockBuilder->CreateStructGEP( saLValue, 2 );
          m_variableArrayAdapter->llvmInit( basicBlockBuilder, vaLValue );
          m_variableArrayAdapter->llvmCallResize( basicBlockBuilder, vaLValue, sizeRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder(
          moduleBuilder,
          this,
          "that", this,
          "offset", sizeAdapter,
          "size", sizeAdapter
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *dstSlicedArrayLValue = functionBuilder[0];
          llvm::Value *srcSlicedArrayRValue = functionBuilder[1];
          llvm::Value *offsetRValue = functionBuilder[2];
          llvm::Value *sizeRValue = functionBuilder[3];
        
          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *offsetPlusSizeInRangeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "offsetPlusSizeInRange" );
          llvm::BasicBlock *offsetPlusSizeOutOfRangeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "offsetPlusSizeOutOfRange" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *srcSizeLValue = basicBlockBuilder->CreateStructGEP( srcSlicedArrayRValue, 1 );
          llvm::Value *srcSizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, srcSizeLValue );
          llvm::Value *offsetPlusSizeRValue = basicBlockBuilder->CreateAdd( offsetRValue, sizeRValue );
          llvm::Value *offsetPlusSizeInRangeCond = basicBlockBuilder->CreateICmpULT( offsetPlusSizeRValue, srcSizeRValue );
          basicBlockBuilder->CreateCondBr( offsetPlusSizeInRangeCond, offsetPlusSizeInRangeBB, offsetPlusSizeOutOfRangeBB );
          
          basicBlockBuilder->SetInsertPoint( offsetPlusSizeInRangeBB );
          llvm::Value *srcOffsetLValue = basicBlockBuilder->CreateStructGEP( srcSlicedArrayRValue, 0 );
          llvm::Value *srcOffsetRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, srcOffsetLValue );
          llvm::Value *combinedOffsetRValue = basicBlockBuilder->CreateAdd( srcOffsetRValue, offsetRValue );
          llvm::Value *offsetLValue = basicBlockBuilder->CreateStructGEP( dstSlicedArrayLValue, 0 );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, offsetLValue, combinedOffsetRValue );
          llvm::Value *sizeLValue = basicBlockBuilder->CreateStructGEP( dstSlicedArrayLValue, 1 );
          sizeAdapter->llvmDefaultAssign( basicBlockBuilder, sizeLValue, sizeRValue );
          llvm::Value *srcVARValue = basicBlockBuilder->CreateStructGEP( srcSlicedArrayRValue, 2 );
          llvm::Value *dstVALValue = basicBlockBuilder->CreateStructGEP( dstSlicedArrayLValue, 2 );
          m_variableArrayAdapter->llvmDefaultAssign(
            basicBlockBuilder,
            dstVALValue,
            srcVARValue
            );
          basicBlockBuilder->CreateRetVoid();

          basicBlockBuilder->SetInsertPoint( offsetPlusSizeOutOfRangeBB );
          llvmThrowOutOfRangeException(
            basicBlockBuilder,
            constStringAdapter->llvmConst( basicBlockBuilder, "offset+size" ),
            offsetPlusSizeRValue,
            srcSizeRValue,
            llvm::ConstantPointerNull::get( static_cast<llvm::PointerType const *>( constStringAdapter->llvmRType( context ) ) )
            );
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
          llvm::Value *arrayLValue = llvmRValueToLValue( basicBlockBuilder, arrayRValue );
          llvm::Value *sizeRValue;
          if ( guarded )
          {
            llvm::Value *sizeLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, 1 );
            sizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, sizeLValue );
            llvm::Value *inRangeCond = basicBlockBuilder->CreateICmpULT( indexRValue, sizeRValue );
            basicBlockBuilder->CreateCondBr( inRangeCond, inRangeBB, outOfRangeBB );
            
            basicBlockBuilder->SetInsertPoint( inRangeBB );
          }
          llvm::Value *offsetLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, 0 );
          llvm::Value *offsetRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, offsetLValue );
          llvm::Value *absoluteIndexRValue = basicBlockBuilder->CreateAdd( offsetRValue, indexRValue );
          llvm::Value *vaLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, 2 );
          llvm::Value *vaRValue = m_variableArrayAdapter->llvmLValueToRValue( basicBlockBuilder, vaLValue );
          basicBlockBuilder->CreateRet(
            m_variableArrayAdapter->llvmConstIndexOp_NoCheck( basicBlockBuilder, vaRValue, absoluteIndexRValue )
            );
          
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
            basicBlockBuilder->CreateUnreachable();
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
            llvm::Value *sizeLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, 1 );
            sizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, sizeLValue );
            llvm::Value *inRangeCond = basicBlockBuilder->CreateICmpULT( indexRValue, sizeRValue );
            basicBlockBuilder->CreateCondBr( inRangeCond, inRangeBB, outOfRangeBB );
            
            basicBlockBuilder->SetInsertPoint( inRangeBB );
          }
          llvm::Value *offsetLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, 0 );
          llvm::Value *offsetRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, offsetLValue );
          llvm::Value *absoluteIndexRValue = basicBlockBuilder->CreateAdd( offsetRValue, indexRValue );
          llvm::Value *vaLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, 2 );
          basicBlockBuilder->CreateRet(
            m_variableArrayAdapter->llvmNonConstIndexOp_NoCheck( basicBlockBuilder, vaLValue, absoluteIndexRValue )
            );
          
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
            basicBlockBuilder->CreateUnreachable();
          }
        }
      }

      {
        ConstructorBuilder functionBuilder( moduleBuilder, booleanAdapter, this, ConstructorBuilder::HighCost );
        if ( buildFunctions )
        {
          llvm::Value *booleanLValue = functionBuilder[0];
          llvm::Value *arrayRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *sizeLValue = basicBlockBuilder->CreateConstGEP2_32( arrayRValue, 0, 1 );
          llvm::Value *sizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, sizeLValue );
          llvm::Value *booleanRValue = basicBlockBuilder->CreateICmpNE( sizeRValue, llvm::ConstantInt::get( sizeRValue->getType(), 0 ) );
          booleanAdapter->llvmAssign( basicBlockBuilder, booleanLValue, booleanRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, stringAdapter, this, ConstructorBuilder::HighCost );
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
          llvm::Value *selfRValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *sizeLValue = basicBlockBuilder->CreateStructGEP( selfRValue, 1 );
          llvm::Value *sizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, sizeLValue );
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
            llvm::Value *selfRValue = functionBuilder[0];
            BasicBlockBuilder basicBlockBuilder( functionBuilder );
            basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
            llvm::Value *sizeLValue = basicBlockBuilder->CreateStructGEP( selfRValue, 1 );
            llvm::Value *sizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, sizeLValue );
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
            llvm::Value *arrayRValue = functionBuilder[0];
            BasicBlockBuilder basicBlockBuilder( functionBuilder );
            basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
            llvm::Value *arrayLValue = llvmRValueToLValue( basicBlockBuilder, arrayRValue );
            llvm::Value *offsetLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, 0 );
            llvm::Value *offsetRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, offsetLValue );
            llvm::Value *vaLValue = basicBlockBuilder->CreateStructGEP( arrayLValue, 2 );
            llvm::Value *memberLValue = m_variableArrayAdapter->llvmNonConstIndexOp_NoCheck( basicBlockBuilder, vaLValue, offsetRValue );
            basicBlockBuilder->CreateRet(
              basicBlockBuilder->CreatePointerCast(
                memberLValue,
                dataAdapter->llvmRType(context)
                )
              );
          }
        }
      }
    }
   
    void *SlicedArrayAdapter::llvmResolveExternalFunction( std::string const &functionName ) const
    {
      return ArrayAdapter::llvmResolveExternalFunction( functionName );
    }

    llvm::Value *SlicedArrayAdapter::llvmConstIndexOp(
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
        params.push_back( FunctionParam( "constString", constStringAdapter, CG::USAGE_RVALUE ) );
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


    llvm::Value *SlicedArrayAdapter::llvmNonConstIndexOp(
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
        params.push_back( FunctionParam( "constString", constStringAdapter, CG::USAGE_RVALUE ) );
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

    void SlicedArrayAdapter::llvmDisposeImpl( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      
      llvm::Value *vaLValue = basicBlockBuilder->CreateStructGEP( lValue, 2 );
      m_variableArrayAdapter->llvmDispose( basicBlockBuilder, vaLValue );
    }
    
    llvm::Constant *SlicedArrayAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      std::vector<llvm::Constant *> elementDefaultRValues;
      elementDefaultRValues.push_back( sizeAdapter->llvmDefaultValue( basicBlockBuilder ) );
      elementDefaultRValues.push_back( sizeAdapter->llvmDefaultValue( basicBlockBuilder ) );
      elementDefaultRValues.push_back( m_variableArrayAdapter->llvmDefaultValue( basicBlockBuilder ) );
      return llvm::ConstantStruct::get( (llvm::StructType const *)llvmRawType( basicBlockBuilder.getContext() ), elementDefaultRValues );
    }
      
    llvm::Constant *SlicedArrayAdapter::llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmDefaultLValue( basicBlockBuilder );
    }

    llvm::Constant *SlicedArrayAdapter::llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const
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

    void SlicedArrayAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();

      sizeAdapter->llvmDefaultAssign(
        basicBlockBuilder,
        basicBlockBuilder->CreateStructGEP( dstLValue, 0 ),
        basicBlockBuilder->CreateStructGEP( srcRValue, 0 )
        );
      sizeAdapter->llvmDefaultAssign(
        basicBlockBuilder,
        basicBlockBuilder->CreateStructGEP( dstLValue, 1 ),
        basicBlockBuilder->CreateStructGEP( srcRValue, 1 )
        );
      m_variableArrayAdapter->llvmDefaultAssign(
        basicBlockBuilder,
        basicBlockBuilder->CreateStructGEP( dstLValue, 2 ),
        basicBlockBuilder->CreateStructGEP( srcRValue, 2 )
        );
    }
  }
}
