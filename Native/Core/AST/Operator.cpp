/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Operator.h"

#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/AST/Param.h>

#include <llvm/Module.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( Operator );
    
    RC::ConstHandle<Function> Operator::Create(
      CG::Location const &location,
      std::string const &functionName,
      RC::ConstHandle<ParamVector> const &params,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body
      )
    {
      return new Operator( location, functionName, params, symbolName, body );
    }
  
    Operator::Operator( 
      CG::Location const &location,
      std::string const &functionName,
      RC::ConstHandle<ParamVector> const &params,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body
      )
      : Function( location, "", functionName, params, symbolName, body, true )
    {
    }

    std::string Operator::getStubName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return getDefaultSymbolName( cgManager ) + ".stub";
    }

    void Operator::llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const
    {
      Function::llvmCompileToModule( moduleBuilder, diagnostics, buildFunctionBodies );

      RC::Handle<CG::Context> context = moduleBuilder.getContext();
      RC::Handle<CG::Manager> cgManager = moduleBuilder.getManager();

      RC::ConstHandle<ParamVector> params = getParams( cgManager );
      CG::AdapterVector paramAdapters = params->getAdapters( cgManager );
      size_t numArgs = params->size();

      std::vector<llvm::Type const *> argTypes;
      llvm::Type const *int64Type = llvm::Type::getInt64Ty( context->getLLVMContext() );
      llvm::Type const *ptrType = llvm::Type::getInt8PtrTy( context->getLLVMContext() );
      llvm::Type const *ptrArrayType = llvm::ArrayType::get( ptrType, numArgs );
      llvm::Type const *int64ArrayType = llvm::ArrayType::get( int64Type, numArgs );

      argTypes.push_back( int64Type ); // start
      argTypes.push_back( int64Type ); // count
      argTypes.push_back( llvm::PointerType::getUnqual( ptrArrayType ) ); // bases
      argTypes.push_back( llvm::PointerType::getUnqual( int64ArrayType ) ); // strides

      llvm::FunctionType const *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );

      std::string name = getStubName( cgManager );
      llvm::Function *func = llvm::cast<llvm::Function>( moduleBuilder->getFunction( name ) );
      if ( !func )
      {
        llvm::AttributeWithIndex AWI[1];
        AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint );
        llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );

        func = llvm::cast<llvm::Function>( moduleBuilder->getOrInsertFunction( name, funcType, attrListPtr ) );
        func->setLinkage( llvm::GlobalValue::ExternalLinkage );

        CG::FunctionBuilder functionBuilder( moduleBuilder, funcType, func );
        llvm::Argument *start = functionBuilder[0];
        start->setName( "start" );
        llvm::Argument *count = functionBuilder[1];
        count->setName( "count" );
        llvm::Argument *bases = functionBuilder[2];
        bases->setName( "bases" );
        llvm::Argument *strides = functionBuilder[3];
        strides->setName( "strides" );

        CG::BasicBlockBuilder bbb( functionBuilder );
        llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
        llvm::BasicBlock *loopCmpBB = functionBuilder.createBasicBlock( "loopCmp" );
        llvm::BasicBlock *loopBodyBB = functionBuilder.createBasicBlock( "loopBody" );
        llvm::BasicBlock *doneBB = functionBuilder.createBasicBlock( "done" );

        bbb->SetInsertPoint( entryBB );
        RC::ConstHandle<CG::SizeAdapter> sizeAdapter = cgManager->getSizeAdapter();
        llvm::Value *zeroRValue = sizeAdapter->llvmConst( context, 0 );
        llvm::Value *oneRValue = sizeAdapter->llvmConst( context, 1 );
        llvm::Value *numArgsRValue = sizeAdapter->llvmConst( context, numArgs );

        llvm::Value *startRValue = llvm::cast<llvm::Value>( start );
        llvm::Value *countRValue = llvm::cast<llvm::Value>( count );
        llvm::Value *basesLValue = llvm::cast<llvm::Value>( bases );
        llvm::Value *stridesLValue = llvm::cast<llvm::Value>( strides );

        llvm::Value *endRValue = bbb->CreateAdd( startRValue, countRValue, "endRValue" );

        llvm::Value *stepLValue = sizeAdapter->llvmAlloca( bbb, "stepLValue" );
        llvm::Value *indexLValue = sizeAdapter->llvmAlloca( bbb, "indexLValue" );
        sizeAdapter->llvmAssign( bbb, indexLValue, startRValue );
        bbb->CreateBr( loopCmpBB );

        bbb->SetInsertPoint( loopCmpBB );
        bbb->CreateCondBr( bbb->CreateICmpULT( sizeAdapter->llvmLValueToRValue( bbb, indexLValue ), endRValue ), loopBodyBB, doneBB );

        bbb->SetInsertPoint( loopBodyBB );

        llvm::Function *realOp = llvm::cast<llvm::Function>( moduleBuilder->getFunction( getSymbolName( cgManager ) ) );
        std::vector<llvm::Value *>args;
        for (int argIndex=0; argIndex<numArgs; argIndex++)
        {
          llvm::BasicBlock *getStrideBB = functionBuilder.createBasicBlock( "getStrideBB" );
          llvm::BasicBlock *getValueBB = functionBuilder.createBasicBlock( "getValueBB" );

          // bases[argIndex] + index*strides[argIndex]
          llvm::Value *argIndexRValue = sizeAdapter->llvmConst( context, argIndex );
          std::vector<llvm::Value *> argArrayIdx;
          argArrayIdx.push_back( zeroRValue );
          argArrayIdx.push_back( argIndexRValue );

          llvm::Value *argBaseLValue = bbb->CreateGEP( basesLValue, argArrayIdx.begin(), argArrayIdx.end(), "argBaseLValue" );
          llvm::Value *argBaseRValue = bbb->CreateLoad( argBaseLValue, "argBaseRValue" );
          sizeAdapter->llvmAssign( bbb, stepLValue, zeroRValue );
          bbb->CreateCondBr( bbb->CreateIsNotNull( stridesLValue ), getStrideBB, getValueBB );

          bbb->SetInsertPoint( getStrideBB );
          llvm::Value *argStrideLValue = bbb->CreateGEP( stridesLValue, argArrayIdx.begin(), argArrayIdx.end(), "argStrideLValue" );
          llvm::Value *stepRValue = bbb->CreateMul(
            sizeAdapter->llvmLValueToRValue( bbb, argStrideLValue ),
            sizeAdapter->llvmLValueToRValue( bbb, indexLValue ),
            "stepRValue"
          );
          sizeAdapter->llvmAssign( bbb, stepLValue, stepRValue );
          bbb->CreateBr( getValueBB );

          bbb->SetInsertPoint( getValueBB );
          llvm::Value *argLValue = bbb->CreateGEP(
            argBaseRValue,
            sizeAdapter->llvmLValueToRValue( bbb, stepLValue ),
            "argLValue" );
          const llvm::Type *argType = paramAdapters[argIndex]->llvmRawType( context );
          llvm::Value *argValue;
          if ( paramAdapters[argIndex]->isPassByReference() || params->get(argIndex)->getUsage() == CG::USAGE_LVALUE )
          {
            argValue = bbb->CreatePointerCast(
              argLValue,
              argType->getPointerTo(),
              "argValue"
            );
          }
          else if ( params->get(argIndex)->getName() == "index" )
          {
            argValue = bbb->CreatePointerCast(
              argLValue,
              argType,
              "argValue"
            );
          }
          else
          {
            // andrew 2012-03-23
            // FIXME dummy value, the only allowed r-value parameter in DG
            // operators is the special 'index' parameter so this is never
            // actually called (will fix when MR stubs are implemented)
            argValue = paramAdapters[argIndex]->llvmDefaultRValue( bbb );
          }

          args.push_back( argValue );
        }
        bbb->CreateCall( realOp, args.begin(), args.end() );

        llvm::Value *nextIndexRValue = bbb->CreateAdd(
          sizeAdapter->llvmLValueToRValue( bbb, indexLValue ),
          oneRValue,
          "nextIndexLValue"
        );
        sizeAdapter->llvmAssign( bbb, indexLValue,  nextIndexRValue );
        bbb->CreateBr( loopCmpBB );

        bbb->SetInsertPoint( doneBB );
        bbb->CreateRetVoid();
      }
    }
  }
}
