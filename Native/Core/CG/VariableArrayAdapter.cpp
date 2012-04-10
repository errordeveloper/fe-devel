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
    
    llvm::Type *VariableArrayAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::Type *llvmSizeTy = llvmSizeType( context );
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
      
      std::vector<llvm::Type *> memberLLVMTypes;
      memberLLVMTypes.push_back( llvmSizeTy ); // refCount
      memberLLVMTypes.push_back( llvmSizeTy ); // allocNumMembers
      memberLLVMTypes.push_back( llvmSizeTy ); // numMembers
      memberLLVMTypes.push_back( llvm::ArrayType::get( m_memberAdapter->llvmRawType( context ), 0 ) );
      llvm::Type *implType = llvm::StructType::create( llvmContext, memberLLVMTypes, getCodeName() + ".Bits", true );
      
      return implType->getPointerTo();
    }
    
    llvm::Type *VariableArrayAdapter::getLLVMImplType( RC::Handle<Context> const &context ) const
    {
      return static_cast<llvm::PointerType *>( llvmRawType( context ) )->getElementType();
    }
    
    void VariableArrayAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
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
          llvm::BasicBlock *nonNullBB = functionBuilder.createBasicBlock( "nonNull" );
          llvm::BasicBlock *nullBB = functionBuilder.createBasicBlock( "null" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *bitsLValue = basicBlockBuilder->CreateLoad( arrayRValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateIsNotNull( bitsLValue ),
            nonNullBB,
            nullBB
            );

          basicBlockBuilder->SetInsertPoint( nonNullBB );
          llvm::Value *sizeLValue = basicBlockBuilder->CreateStructGEP( bitsLValue, SizeIndex );
          llvm::Value *sizeRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, sizeLValue );
          basicBlockBuilder->CreateRet( sizeRValue );

          basicBlockBuilder->SetInsertPoint( nullBB );
          sizeRValue = sizeAdapter->llvmConst( context, 0 );
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
          llvm::BasicBlock *differentBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "different" );
          llvm::BasicBlock *doneBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "done" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *dstBits = basicBlockBuilder->CreateLoad( dstLValue );
          llvm::Value *srcBits = basicBlockBuilder->CreateLoad( srcRValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpNE(
              dstBits,
              srcBits
              ),
            differentBB,
            doneBB
            );

          basicBlockBuilder->SetInsertPoint( differentBB );
          llvmRelease( basicBlockBuilder, dstBits );
          basicBlockBuilder->CreateStore( srcBits, dstLValue );
          llvmRetain( basicBlockBuilder, srcBits );
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

          basicBlockBuilder->SetInsertPoint( entryBB );
          llvmCallResize( basicBlockBuilder, selfLValue, newSizeRValue );
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
            basicBlockBuilder->CreateUnreachable();
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
          llvmPrepareForModify( basicBlockBuilder, arrayLValue );
          llvm::Value *bitsLValue = basicBlockBuilder->CreateLoad( arrayLValue );
          llvm::Value *memberDatasLValue = basicBlockBuilder->CreateStructGEP( basicBlockBuilder->CreateStructGEP( bitsLValue, MemberDatasIndex ), 0 );
          llvm::Value *memberLValue = basicBlockBuilder->CreateGEP( memberDatasLValue, indexRValue );
          basicBlockBuilder->CreateRet( memberLValue );
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
          llvm::Value *sizeRValue = llvmCallSize( basicBlockBuilder, arrayRValue );
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
      
      llvm::Function *assignAddFunction;
      {
        std::vector< llvm::Type * > argTypes;
        argTypes.push_back( llvm::Type::getInt8PtrTy( context->getLLVMContext() ) );
        argTypes.push_back( llvmLType( context ) );
        argTypes.push_back( llvmRType( context ) );
        llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
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
          basicBlockBuilder->CreateCall( func, args );
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
            llvm::Value *bits = basicBlockBuilder->CreateLoad( selfRValue );
            llvm::Value *memberDatasLValue = basicBlockBuilder->CreateStructGEP( basicBlockBuilder->CreateStructGEP( bits, MemberDatasIndex ), 0 );
            basicBlockBuilder->CreateRet(
              basicBlockBuilder->CreatePointerCast( memberDatasLValue, dataAdapter->llvmRType( context ) )
              );
          }
        }
      }
    }
    
    void *VariableArrayAdapter::llvmResolveExternalFunction( std::string const &functionName ) const
    {
      if ( functionName == "__"+getCodeName()+"__Resize" )
        return (void *)&VariableArrayAdapter::Resize;
      else if ( functionName == "__"+getCodeName()+"__Pop" )
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
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), args );
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
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), args );
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
      inst->m_variableArrayImpl->append( dstLValue, srcRValue );
    }
    
    void VariableArrayAdapter::Pop( VariableArrayAdapter const *inst, void *dst, void *result )
    {
      inst->m_variableArrayImpl->pop( dst, result );
    }
    
    void VariableArrayAdapter::Resize( VariableArrayAdapter const *inst, void *data, size_t newSize )
    {
      inst->m_variableArrayImpl->setNumMembers( data, newSize );
    }

    void VariableArrayAdapter::llvmCallResize( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayLValue, llvm::Value *newSizeRValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      
      std::vector< llvm::Type * > argTypes;
      argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
      argTypes.push_back( llvmLType( context ) );
      argTypes.push_back( sizeAdapter->llvmRType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
      llvm::AttributeWithIndex AWI[3];
      AWI[0] = llvm::AttributeWithIndex::get( 1, llvm::Attribute::NoCapture | llvm::Attribute::NoAlias );
      AWI[1] = llvm::AttributeWithIndex::get( 2, llvm::Attribute::NoCapture | llvm::Attribute::NoAlias );
      AWI[2] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 3 );
      
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"__Resize", funcType, attrListPtr ) ); 

      std::vector< llvm::Value * > args;
      args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
      args.push_back( arrayLValue );
      args.push_back( newSizeRValue );
      basicBlockBuilder->CreateCall( func, args );
    }

    void VariableArrayAdapter::llvmCallPop( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayLValue, llvm::Value *memberLValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      
      std::vector< llvm::Type * > argTypes;
      argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
      argTypes.push_back( llvmLType( context ) );
      argTypes.push_back( m_memberAdapter->llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
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
      basicBlockBuilder->CreateCall( func, args );
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

      std::vector<llvm::Type *> argTypes;
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
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
        
        bbb->SetInsertPoint( entryBB );
        llvm::Value *bitsLValue = bbb->CreateLoad( selfLValue );
        llvmRelease( bbb, bitsLValue );
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
      return llvm::ConstantPointerNull::get( static_cast<llvm::PointerType *>( llvmRawType( basicBlockBuilder.getContext() ) ) );
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

      std::vector<llvm::Type *> argTypes;
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
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
        bbb->CreateStore(
          llvm::ConstantPointerNull::get( static_cast<llvm::PointerType *>( llvmRawType( basicBlockBuilder.getContext() ) ) ),
          selfLValue
          );
        bbb->CreateRetVoid();
      }
      
      basicBlockBuilder->CreateCall( func, selfLValue );
    }

    void VariableArrayAdapter::llvmPrepareForModify( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayLValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();

      std::vector<llvm::Type *> argTypes;
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );

      std::string name = "__" + getCodeName() + "__PrepareForModify";
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
        llvm::BasicBlock *isSharedBB = fb.createBasicBlock( "isShared" );
        llvm::BasicBlock *doneBB = fb.createBasicBlock( "done" );

        bbb->SetInsertPoint( entryBB );
        llvm::Value *bitsLValue = bbb->CreateLoad( selfLValue );
        llvm::Value *refCountLValue = bbb->CreateStructGEP( bitsLValue, RefCountIndex );
        llvm::Value *refCountRValue = sizeAdapter->llvmLValueToRValue( bbb, refCountLValue );
        bbb->CreateCondBr(
          bbb->CreateICmpUGT(
            refCountRValue,
            sizeAdapter->llvmConst( context, 1 )
            ),
          isSharedBB,
          doneBB
          );

        bbb->SetInsertPoint( isSharedBB );
        llvmDuplicate( bbb, selfLValue );
        bbb->CreateBr( doneBB );

        bbb->SetInsertPoint( doneBB );
        bbb->CreateRetVoid();
      }
      
      basicBlockBuilder->CreateCall( func, arrayLValue );
    }

    void VariableArrayAdapter::llvmDuplicate( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayLValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      
      std::vector< llvm::Type * > argTypes;
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );

      std::string name = "__"+getCodeName()+"__Duplicate";
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
        llvm::BasicBlock *loopCheckBB = fb.createBasicBlock( "loopCheck" );
        llvm::BasicBlock *loopBodyBB = fb.createBasicBlock( "loopBodyBB" );
        llvm::BasicBlock *loopDoneBB = fb.createBasicBlock( "loopDoneBB" );
        
        bbb->SetInsertPoint( entryBB );
        llvm::Value *oldBitsLValue = bbb->CreateLoad( selfLValue );
        llvm::Value *allocSizeRValue = sizeAdapter->llvmLValueToRValue(
          bbb,
          bbb->CreateStructGEP(
            oldBitsLValue,
            AllocSizeIndex
            )
          );
        llvm::Value *mallocSizeRValue = bbb->CreateAdd(
          sizeAdapter->llvmConst( context, 3 * sizeof(size_t) ),
          bbb->CreateMul(
            allocSizeRValue,
            sizeAdapter->llvmConst( context, m_variableArrayImpl->getMemberImpl()->getAllocSize() )
            )
          );
        llvm::Value *newBitsLValue = bbb->CreatePointerCast(
          llvmCallMalloc(
            bbb,
            mallocSizeRValue
            ),
          static_cast<llvm::PointerType *>( oldBitsLValue->getType() )
          );
        sizeAdapter->llvmDefaultAssign(
          bbb,
          bbb->CreateStructGEP(
            newBitsLValue,
            RefCountIndex
            ),
          sizeAdapter->llvmConst( context, 1 )
          );
        sizeAdapter->llvmDefaultAssign(
          bbb,
          bbb->CreateStructGEP(
            newBitsLValue,
            AllocSizeIndex
            ),
          allocSizeRValue
          );
        llvm::Value *sizeRValue = sizeAdapter->llvmLValueToRValue(
          bbb,
          bbb->CreateStructGEP(
            oldBitsLValue,
            SizeIndex
            )
          );
        sizeAdapter->llvmDefaultAssign(
          bbb,
          bbb->CreateStructGEP(
            newBitsLValue,
            SizeIndex
            ),
          sizeRValue
          );
        llvm::Value *indexLValue = sizeAdapter->llvmAlloca( bbb, "index" );
        sizeAdapter->llvmDefaultAssign(
          bbb,
          indexLValue,
          sizeAdapter->llvmConst( context, 0 )
          );
        bbb->CreateBr( loopCheckBB );

        bbb->SetInsertPoint( loopCheckBB );
        llvm::Value *indexRValue = sizeAdapter->llvmLValueToRValue( bbb, indexLValue );
        bbb->CreateCondBr(
          bbb->CreateICmpULT(
            indexRValue,
            sizeRValue
            ),
          loopBodyBB,
          loopDoneBB
          );

        bbb->SetInsertPoint( loopBodyBB );
        llvm::Value *dstMemberLValue = bbb->CreateGEP( bbb->CreateStructGEP( bbb->CreateStructGEP( newBitsLValue, MemberDatasIndex ), 0 ), indexRValue );
        m_memberAdapter->llvmInit( bbb, dstMemberLValue );
        m_memberAdapter->llvmAssign( 
          bbb,
          dstMemberLValue,
          m_memberAdapter->llvmLValueToRValue( bbb, bbb->CreateGEP( bbb->CreateStructGEP( bbb->CreateStructGEP( oldBitsLValue, MemberDatasIndex ), 0 ), indexRValue ) )
          );
        sizeAdapter->llvmDefaultAssign(
          bbb,
          indexLValue,
          bbb->CreateAdd(
            indexRValue,
            sizeAdapter->llvmConst( context, 1 )
            )
          );
        bbb->CreateBr( loopCheckBB );

        bbb->SetInsertPoint( loopDoneBB );
        llvmRelease( bbb, oldBitsLValue );
        bbb->CreateStore( newBitsLValue, selfLValue );
        bbb->CreateRetVoid();
      }

      std::vector< llvm::Value * > args;
      args.push_back( arrayLValue );
      basicBlockBuilder->CreateCall( func, args );
    }
    
    void VariableArrayAdapter::llvmRetain( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *bitsLValue ) const
    {    
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();

      std::vector<llvm::Type *> argTypes;
      argTypes.push_back( bitsLValue->getType() );
      llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );

      std::string name = "__"+getCodeName()+"__Retain";
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
        bitsLValue->addAttr( llvm::Attribute::NoAlias );
        
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
    
    void VariableArrayAdapter::llvmRelease( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *bitsLValue ) const
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
        bitsLValue->addAttr( llvm::Attribute::NoAlias );
        
        BasicBlockBuilder bbb( fb );

        llvm::BasicBlock *entryBB = fb.createBasicBlock( "entry" );
        llvm::BasicBlock *nonNullBB = fb.createBasicBlock( "nonNull" );
        llvm::BasicBlock *freeBB = fb.createBasicBlock( "free" );
        llvm::BasicBlock *checkBB = fb.createBasicBlock( "check" );
        llvm::BasicBlock *nextBB = fb.createBasicBlock( "next" );
        llvm::BasicBlock *doneFreeBB = fb.createBasicBlock( "doneFree" );
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

        bbb->CreateCondBr(
          bbb->CreateICmpEQ(
            oldRefCountRValue,
            oneRValue
            ),
          freeBB,
          doneBB
          );

        bbb->SetInsertPoint( freeBB );
        llvm::Value *sizeLValue = bbb->CreateStructGEP( bitsLValue, SizeIndex );
        llvm::Value *sizeRValue = sizeAdapter->llvmLValueToRValue( bbb, sizeLValue );
        llvm::Value *memberDatasLValue = bbb->CreateStructGEP( bbb->CreateStructGEP( bitsLValue, MemberDatasIndex ), 0 );
        llvm::Value *indexLValue = sizeAdapter->llvmAlloca( bbb, "index" );
        sizeAdapter->llvmInit( bbb, indexLValue );
        bbb->CreateBr( checkBB );
        
        bbb->SetInsertPoint( checkBB );
        llvm::Value *indexRValue = sizeAdapter->llvmLValueToRValue( bbb, indexLValue );
        bbb->CreateCondBr(
          bbb->CreateICmpULT( indexRValue, sizeRValue ),
          nextBB,
          doneFreeBB
          );
          
        bbb->SetInsertPoint( nextBB );
        llvm::Value *memberLValue = bbb->CreateGEP( memberDatasLValue, indexRValue );
        m_memberAdapter->llvmDispose( bbb, memberLValue );
        llvm::Value *nextIndexRValue = bbb->CreateAdd( indexRValue, sizeAdapter->llvmConst( context, 1 ) );
        sizeAdapter->llvmDefaultAssign( bbb, indexLValue, nextIndexRValue );
        bbb->CreateBr( checkBB );
          
        bbb->SetInsertPoint( doneFreeBB );
        llvmCallFree( bbb, bitsLValue );
        bbb->CreateBr( doneBB );

        bbb->SetInsertPoint( doneBB );
        bbb->CreateRetVoid();
      }

      std::vector<llvm::Value *> args;
      args.push_back( bitsLValue );
      basicBlockBuilder->CreateCall( func, args );
    }
  }
}
