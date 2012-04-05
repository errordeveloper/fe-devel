/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "DictAdapter.h"
#include "BooleanAdapter.h"
#include "ComparableAdapter.h"
#include "IntegerAdapter.h"
#include "SizeAdapter.h"
#include "ConstStringAdapter.h"
#include "StringAdapter.h"
#include "OpaqueAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "BasicBlockBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/RT/DictDesc.h>
#include <Fabric/Core/RT/DictImpl.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Intrinsics.h>
#include <llvm/Type.h>

namespace Fabric
{
  namespace CG
  {
    DictAdapter::DictAdapter(
      RC::ConstHandle<Manager> const &manager,
      RC::ConstHandle<RT::DictDesc> const &dictDesc
      )
      : Adapter( manager, dictDesc, FL_PASS_BY_REFERENCE )
      , m_dictDesc( dictDesc )
      , m_keyAdapter( RC::ConstHandle<ComparableAdapter>::StaticCast( manager->getAdapter( dictDesc->getKeyDesc() ) ) )
      , m_valueAdapter( manager->getAdapter( dictDesc->getValueDesc() ) )
      , m_dictImpl( dictDesc->getImpl().ptr() )
    {
    }
    
    llvm::Type *DictAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::LLVMContext &llvmContext = context->getLLVMContext();
      
      llvm::Type *llvmSizeTy = llvmSizeType( context );
      
      /*
      struct node_t
      {
        node_t *bitsPrevNode;
        node_t *bitsNextNode;
        node_t *bucketPrevNode;
        node_t *bucketNextNode;
        size_t keyHash;
        char keyAndValue[0];
      };
      */
      
      llvm::StructType *llvmAbstractNodeType = llvm::StructType::get( llvmContext, true );
      std::vector<llvm::Type *> llvmNodeTypeMembers;
      llvmNodeTypeMembers.push_back( llvmAbstractNodeType->getPointerTo() ); // bitsPrevNode
      llvmNodeTypeMembers.push_back( llvmAbstractNodeType->getPointerTo() ); // bitsNextNode
      llvmNodeTypeMembers.push_back( llvmAbstractNodeType->getPointerTo() ); // bucketPrevNode
      llvmNodeTypeMembers.push_back( llvmAbstractNodeType->getPointerTo() ); // bucketNextNode
      llvmNodeTypeMembers.push_back( llvmSizeTy ); // keyHash
      llvmNodeTypeMembers.push_back( llvm::ArrayType::get( llvm::Type::getInt8Ty( llvmContext ), 0 ) );
      llvm::StructType *llvmNodeType = llvm::StructType::create( llvmContext, llvmNodeTypeMembers, getCodeName() + ".Node", true );

      //Note: this no longer exists in LLVM3.0 and actually asserted because llvmAbstractNodeType is not abstract.
      //I think it was doing nothing in fact...?
      //llvmNodeType->refineAbstractType( llvmAbstractNodeType, llvmNodeType );
      
      /*
      struct bucket_t
      {
        node_t *firstNode;
        node_t *lastNode;
      };
      */

      std::vector<llvm::Type *> llvmBucketTypeMembers;
      llvmBucketTypeMembers.push_back( llvmNodeType->getPointerTo() ); // firstNode
      llvmBucketTypeMembers.push_back( llvmNodeType->getPointerTo() ); // lastNode
      llvm::Type *llvmBucketType = llvm::StructType::create( llvmContext, llvmBucketTypeMembers, getCodeName() + ".Bucket", true );
      
      /*
      struct bits_t
      {
        size_t bucketCount;
        size_t nodeCount;
        node_t *firstNode;
        node_t *lastNode;
        bucket_t *buckets;
      };
      */

      std::vector<llvm::Type *> llvmBitsTypeMembers;
      llvmBitsTypeMembers.push_back( llvmSizeTy ); // refCount
      llvmBitsTypeMembers.push_back( llvmSizeTy ); // bucketCount
      llvmBitsTypeMembers.push_back( llvmSizeTy ); // nodeCount
      llvmBitsTypeMembers.push_back( llvmNodeType->getPointerTo() ); // numMembers
      llvmBitsTypeMembers.push_back( llvmNodeType->getPointerTo() ); // numMembers
      llvmBitsTypeMembers.push_back( llvmBucketType->getPointerTo() ); // numMembers
      llvm::Type *implType = llvm::StructType::create( llvmContext, llvmBitsTypeMembers, getCodeName() + ".Bits", true );
      
      return implType->getPointerTo();
    }

    void DictAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
      
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      m_keyAdapter->llvmCompileToModule( moduleBuilder );
      m_valueAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );

      static const bool buildFunctions = true;
      
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

          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          llvm::BasicBlock *notNullBB = functionBuilder.createBasicBlock( "notNull" );
          llvm::BasicBlock *nullBB = functionBuilder.createBasicBlock( "null" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *bitsLValue = basicBlockBuilder->CreateLoad( rValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateIsNotNull( bitsLValue ),
            notNullBB,
            nullBB
            );

          basicBlockBuilder->SetInsertPoint( notNullBB );
          llvm::Value *nodeCountLValue = basicBlockBuilder->CreateStructGEP( bitsLValue, 2 );
          llvm::Value *nodeCountRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, nodeCountLValue );
          basicBlockBuilder->CreateRet( nodeCountRValue );

          basicBlockBuilder->SetInsertPoint( nullBB );
          nodeCountRValue = sizeAdapter->llvmConst( context, 0 );
          basicBlockBuilder->CreateRet( nodeCountRValue );
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          booleanAdapter,
          this, USAGE_RVALUE,
          "has",
          "key", m_keyAdapter, CG::USAGE_RVALUE
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          
          llvm::Value *dictRValue = functionBuilder[0];
          llvm::Value *keyRValue = functionBuilder[1];

          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );

          std::vector< llvm::Type * > argTypes;
          argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
          argTypes.push_back( llvmLType( context ) );
          argTypes.push_back( m_keyAdapter->llvmLType( context ) );
          llvm::FunctionType *funcType = llvm::FunctionType::get( booleanAdapter->llvmRType( context ), argTypes, false );
          
          llvm::AttributeWithIndex AWI[1];
          AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
          llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );
          
          llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"_Has", funcType, attrListPtr ) ); 

          llvm::Value *dictLValue = llvmRValueToLValue( basicBlockBuilder, dictRValue );
          llvm::Value *keyLValue = m_keyAdapter->llvmRValueToLValue( basicBlockBuilder, keyRValue );

          std::vector<llvm::Value *> args;
          args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
          args.push_back( dictLValue );
          args.push_back( keyLValue );
          basicBlockBuilder->CreateRet(
            basicBlockBuilder->CreateCall( func, args )
            );
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          0,
          this, USAGE_LVALUE,
          "delete",
          "key", m_keyAdapter, CG::USAGE_RVALUE
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          
          llvm::Value *dictLValue = functionBuilder[0];
          llvm::Value *keyRValue = functionBuilder[1];

          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );

          std::vector< llvm::Type * > argTypes;
          argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
          argTypes.push_back( llvmLType( context ) );
          argTypes.push_back( m_keyAdapter->llvmLType( context ) );
          llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
          
          llvm::AttributeWithIndex AWI[1];
          AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
          llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );
          
          llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"_Delete", funcType, attrListPtr ) ); 

          llvm::Value *keyLValue = m_keyAdapter->llvmRValueToLValue( basicBlockBuilder, keyRValue );

          std::vector<llvm::Value *> args;
          args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
          args.push_back( dictLValue );
          args.push_back( keyLValue );
          basicBlockBuilder->CreateCall( func, args );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        MethodBuilder functionBuilder(
          moduleBuilder,
          0,
          this, USAGE_LVALUE,
          "clear"
          );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          
          llvm::Value *dictLValue = functionBuilder[0];

          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );

          std::vector< llvm::Type * > argTypes;
          argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
          argTypes.push_back( llvmLType( context ) );
          llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
          
          llvm::AttributeWithIndex AWI[1];
          AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
          llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );
          
          llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"_Clear", funcType, attrListPtr ) ); 

          std::vector<llvm::Value *> args;
          args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
          args.push_back( dictLValue );
          basicBlockBuilder->CreateCall( func, args );
          basicBlockBuilder->CreateRetVoid();
        }
      }

      {
        ConstructorBuilder functionBuilder( moduleBuilder, booleanAdapter, this, ConstructorBuilder::HighCost );
        if ( buildFunctions )
        {
          llvm::Value *booleanLValue = functionBuilder[0];
          llvm::Value *rValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *sizeRValue = llvmCallSize( basicBlockBuilder, rValue );
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
          llvm::Value *rValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *lValue = llvmRValueToLValue( basicBlockBuilder, rValue );
          stringAdapter->llvmCallCast( basicBlockBuilder, this, lValue, stringLValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }

      /*
      {
        ParamVector params;
        params.push_back( FunctionParam( "lValue", this, CG::USAGE_LVALUE ) );
        params.push_back( FunctionParam( "keyRValue", m_keyAdapter, CG::USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, "__"+getCodeName()+"__GetLValue", ExprType( m_valueAdapter, USAGE_LVALUE ), params, false, 0, true );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *lValue = functionBuilder[0];
          llvm::Value *keyRValue = functionBuilder[1];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *resizeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "resize" );
          llvm::BasicBlock *accessBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "access" );

      bits_t *bits = reinterpret_cast<bits_t *>( data );
      if ( bits->bucketCount == 0 )
        adjustBucketCount( bits, RT_DICT_IMPL_MINIMUM_BUCKET_COUNT );
      size_t keyHash = m_keyImpl->hash( keyData );
      size_t bucketIndex = keyHash & (bits->bucketCount - 1);
      bucket_t *bucket = &bits->buckets[bucketIndex];
      void *result = getMutable( bits, bucket, keyData, keyHash );
      size_t newBucketCount = BucketCountForNodeCount( bits->nodeCount );
      if ( newBucketCount > bits->bucketCount )
        adjustBucketCount( bits, newBucketCount );
      return result;

          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *nodeCountRValue = sizeAdapter->llvmLValueToRValue(
            basicBlockBuilder,
            basicBlockBuilder->CreateStructGEP( lValue, 1 )
            );
          llvm::Value *minBucketCountRValue = llvmMinBucketCountForNodeCount( basicBlockBuilder, nodeCountRValue );
          llvm::Value *bucketCountLValue = basicBlockBuilder->CreateStructGEP( lValue, 0 );
          llvm::Value *oldBucketCountRValue = sizeAdapter->llvmLValueToRValue( basicBlockBuilder, bucketCountLValue );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpLT(
              bucketCountLValue,
              minBucketCountRValue
              ),
            resizeBB,
            accessBB
            );
          
          basicBlockBuilder->SetInsertPoint( resizeBB );
          llvm::Value *newBucketCountRValue = llvmCallAdjustBucketCount(
            basicBlockBuilder,
            lValue, 
            sizeAdapter->llvmConst( context, RT_DICT_IMPL_MINIMUM_BUCKET_COUNT )
            );
          basicBlockBuilder->CreateBr( accessBB );
          
          basicBlockBuilder->SetInsertPoint( accessBB );
          llvm::Value *keyHashRValue = llvmHash( basicBlockBuilder, keyRValue );
          llvm::PHINode *bucketCountRVaule = basicBlockBuilder->CreatePHI( sizeAdapter->llvmRType( context ), 2 );
          phi->addIncoming( oldBucketCountRValue, entryBB );
          phi->addIncoming( newBucketCountRValue, resizeBB );
          llvm::Value *bucketIndexRValue = basicBlockBuilder->CreateAnd(
            keyHashRValue,
            basicBlockBuilder->CreateSub(
              bucketCountRVaule,
              sizeAdapter->llvmConst( context, 1 )
              )
            );
          llvm::Value *bucketsPtr = basicBlockBuilder->CreateStructGEP( lValue, 4 );
          llvm::Value *bucketLValue = basicBlockBuilder->CreateConstGEP32_2( bucketsPtr, 0, bucketIndexRValue );
          basicBlockBuilder->CreateRet(
            llvmCallGetLValue_Bucket(
              lValue,
              bucketLValue,
              keyRValue,
              keyHashRValue
              )
            );
        }
      }
      
      {
        std::string name = "__" + getCodeName() + "__MinBucketCountForNodeCount";
        ParamVector params;
        params.push_back( FunctionParam( "nodeCountRValue", sizeAdapter, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType(), params );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *rValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *lValue = llvmRValueToLValue( basicBlockBuilder, rValue );
          llvm::Value *stringRValue = stringAdapter->llvmCallCast( basicBlockBuilder, this, lValue );
          stringAdapter->llvmAssign( basicBlockBuilder, stringLValue, stringRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }

      {
        ParamVector params;
        params.push_back( FunctionParam( "lValue", this, CG::USAGE_LVALUE ) );
        params.push_back( FunctionParam( "bucketLValue", this, CG::USAGE_LVALUE ) );
        params.push_back( FunctionParam( "keyRValue", m_keyAdapter, CG::USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, "__"+getCodeName()+"__GetLValue", ExprType( m_valueAdapter, USAGE_LVALUE ), params, false, 0, true );
        if ( buildFunctions )
        {
          BasicBlockBuilder basicBlockBuilder( functionBuilder );

          llvm::Value *lValue = functionBuilder[0];
          llvm::Value *keyRValue = functionBuilder[1];

          llvm::BasicBlock *entryBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "entry" );
          llvm::BasicBlock *resizeBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "resize" );
          llvm::BasicBlock *accessBB = basicBlockBuilder.getFunctionBuilder().createBasicBlock( "access" );

          basicBlockBuilder->SetInsertPoint( entryBB );
          llvm::Value *oldNodeCountRValue = sizeAdapter->llvmLValueToRValue(
            basicBlockBuilder,
            basicBlockBuilder->CreateStructGEP( lValue, 1 )
            );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateICmpEQ(
              basicBlockBuilder->CreateAnd(
                oldNodeCountRValue,
                basicBlockBuilder->CreateAdd(
                  oldNodeCountRValue,
                  sizeAdapter->llvmConst( context, 1 )
                  )
                ),
              sizeAdapter->llvmConst( context, 0 )
            ),
            maybeResizeBB,
            accessBB
            );
          
          basicBlockBuilder->SetInsertPoint( maybeResizeBB );
          llvm::Value *newBucketCountRValue = llvmMaybeResize(
            basicBlockBuilder,
            lValue
            );
          basicBlockBuilder->CreateBr( accessBB );
          
          basicBlockBuilder->SetInsertPoint( accessBB );
          llvm::Value *keyHashRValue = llvmHash( basicBlockBuilder, keyRValue );
          llvm::Value *bucketCountRVaule = sizeAdapter->llvmLValueToRValue(
            basicBlockBuilder,
            basicBlockBuilder->CreateStructGEP( lValue, 0 )
            );
          llvm::Value *bucketIndexRValue = basicBlockBuilder->CreateAnd(
            keyHashRValue,
            basicBlockBuilder->CreateSub(
              bucketCountRValue,
              sizeAdapter->llvmConst( context, 1 )
              )
            );
          llvm::Value *bucketsPtr = basicBlockBuilder->CreateStructGEP( lValue, 4 );
          llvm::Value *bucketLValue = basicBlockBuilder->CreateConstGEP32_2( bucketsPtr, 0, bucketIndexRValue );
          
          llvm::Value *firstNodeLValue = basicBlockBuilder->CreateLoad(
            basicBlockBuilder->CreateStructGEP( bucketLValue, 0 )
            );
          basicBlockBuilder->CreateBr( loopCheckBB );
          
          basicBlockBuilder->SetInsertPoint( loopCheckBB );
          llvm::PHINode *nodeLValue = basicBlockBuilder->CreatePHI( llvmNodePtrType, 2 );
          nodeLValue->addIncoming( firstNodeLValue, accessBB );
          nodeLValue->addIncoming( nodeLValue, loopStepBB );
          basicBlockBuilder->CreateCondBr(
            basicBlockBuilder->CreateIsNull( nodeLValue ),
            loopDoneBB,
            loopStepBB
            );
          
          basicBlockBuilder->SetInsertPoint( loopStepBB );
          llvm::Value *keyLValue = basicBlockBuilder->CreatePointerCast(
             basicBlockBuilder->CreateConstGEP32_2(
               basicBlockBuilder->CreateStructGEP( nodeLValue, 4 ),
               0,
               0
               ),
              m_keyImpl->
          llvm::Value *cmpRValue = m_keyImpl->llvmCompare(
            basicBlockBuilder,
        }
      }
      */
    }
    
    void *DictAdapter::llvmResolveExternalFunction( std::string const &functionName ) const
    {
      if ( functionName == "__" + getCodeName() + "_Has" )
        return (void *)&DictAdapter::Has;
      else if ( functionName == "__" + getCodeName() + "_GetRValue" )
        return (void *)&DictAdapter::GetRValue;
      else if ( functionName == "__" + getCodeName() + "_GetLValue" )
        return (void *)&DictAdapter::GetLValue;
      else if ( functionName == "__" + getCodeName() + "_Delete" )
        return (void *)&DictAdapter::Delete;
      else if ( functionName == "__" + getCodeName() + "_Clear" )
        return (void *)&DictAdapter::Clear;
      else if ( functionName == "__" + getCodeName() + "_Dispose" )
        return (void *)&DictAdapter::Dispose;
      else if ( functionName == "__" + getCodeName() + "_DefaultAssign" )
        return (void *)&DictAdapter::DefaultAssign;
      else return Adapter::llvmResolveExternalFunction( functionName );
    }

    void DictAdapter::llvmDisposeImpl( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();

      std::vector<llvm::Type *> argTypes;
      argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );
      
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"_Dispose", funcType, attrListPtr ) ); 

      std::vector<llvm::Value *> args;
      args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
      args.push_back( lValue );
      basicBlockBuilder->CreateCall( func, args );
    }

    void DictAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();

      std::vector<llvm::Type *> argTypes;
      argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
      argTypes.push_back( llvmLType( context ) );
      argTypes.push_back( llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( llvm::Type::getVoidTy( context->getLLVMContext() ), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );
      
      llvm::Constant *funcAsConstant = basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"_DefaultAssign", funcType, attrListPtr );
      llvm::Function *func = llvm::cast<llvm::Function>( funcAsConstant ); 

      std::vector<llvm::Value *> args;
      args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
      args.push_back( llvmRValueToLValue( basicBlockBuilder, srcRValue ) );
      args.push_back( dstLValue );
      basicBlockBuilder->CreateCall( func, args );
    }
    
    llvm::Constant *DictAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvm::ConstantPointerNull::get( static_cast<llvm::PointerType *>( llvmRawType( basicBlockBuilder.getContext() ) ) );
    }
      
    llvm::Constant *DictAdapter::llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmDefaultLValue( basicBlockBuilder );
    }

    llvm::Constant *DictAdapter::llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const
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
    
    std::string DictAdapter::toString( void const *data ) const
    {
      return m_dictDesc->descData( data );
    }
    
    llvm::Value *DictAdapter::llvmCallSize( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const
    {
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      MethodBuilder functionBuilder(
        basicBlockBuilder.getModuleBuilder(),
        sizeAdapter,
        this, USAGE_RVALUE,
        "size"
        );
      return basicBlockBuilder->CreateCall( functionBuilder.getLLVMFunction(), rValue );
    }

    llvm::Value *DictAdapter::llvmGetRValue(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *dictRValue,
      llvm::Value *keyRValue
      ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      
      std::vector< llvm::Type * > argTypes;
      argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
      argTypes.push_back( llvmLType( context ) );
      argTypes.push_back( m_keyAdapter->llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( m_valueAdapter->llvmLType( context ), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );
      
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"_GetRValue", funcType, attrListPtr ) ); 

      llvm::Value *dictLValue = llvmRValueToLValue( basicBlockBuilder, dictRValue );
      llvm::Value *keyLValue = m_keyAdapter->llvmRValueToLValue( basicBlockBuilder, keyRValue );

      std::vector< llvm::Value * > args;
      args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
      args.push_back( dictLValue );
      args.push_back( keyLValue );
      llvm::Value *resultLValue = basicBlockBuilder->CreateCall( func, args );
      return m_valueAdapter->llvmLValueToRValue( basicBlockBuilder, resultLValue );
    }

    llvm::Value *DictAdapter::llvmGetLValue(
      CG::BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *dictLValue,
      llvm::Value *keyRValue
      ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      
      std::vector< llvm::Type * > argTypes;
      argTypes.push_back( basicBlockBuilder->getInt8PtrTy() );
      argTypes.push_back( llvmLType( context ) );
      argTypes.push_back( m_keyAdapter->llvmLType( context ) );
      llvm::FunctionType *funcType = llvm::FunctionType::get( m_valueAdapter->llvmLType( context ), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );
      
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getOrInsertFunction( "__"+getCodeName()+"_GetLValue", funcType, attrListPtr ) ); 

      llvm::Value *keyLValue = m_keyAdapter->llvmRValueToLValue( basicBlockBuilder, keyRValue );

      std::vector< llvm::Value * > args;
      args.push_back( llvmAdapterPtr( basicBlockBuilder ) );
      args.push_back( dictLValue );
      args.push_back( keyLValue );
      return basicBlockBuilder->CreateCall( func, args );
    }

    bool DictAdapter::Has( void *_dictAdapter, void const *dictLValue, void const *keyLValue )
    {
      DictAdapter const *dictAdapter = static_cast<DictAdapter const *>( _dictAdapter );
      return dictAdapter->m_dictImpl->has( dictLValue, keyLValue );
    }

    void const *DictAdapter::GetRValue( void *_dictAdapter, void const *dictLValue, void const *keyLValue )
    {
      DictAdapter const *dictAdapter = static_cast<DictAdapter const *>( _dictAdapter );
      return dictAdapter->m_dictImpl->getImmutable( dictLValue, keyLValue );
    }

    void *DictAdapter::GetLValue( void *_dictAdapter, void *dictLValue, void const *keyLValue )
    {
      DictAdapter const *dictAdapter = static_cast<DictAdapter const *>( _dictAdapter );
      return dictAdapter->m_dictImpl->getMutable( dictLValue, keyLValue );
    }

    void DictAdapter::Delete( void *_dictAdapter, void *dictLValue, void const *keyLValue )
    {
      DictAdapter const *dictAdapter = static_cast<DictAdapter const *>( _dictAdapter );
      dictAdapter->m_dictImpl->delete_( dictLValue, keyLValue );
    }

    void DictAdapter::Clear( void *_dictAdapter, void *dictLValue )
    {
      DictAdapter const *dictAdapter = static_cast<DictAdapter const *>( _dictAdapter );
      dictAdapter->m_dictImpl->clear( dictLValue );
    }

    void DictAdapter::Dispose( void *_dictAdapter, void *dictLValue )
    {
      DictAdapter const *dictAdapter = static_cast<DictAdapter const *>( _dictAdapter );
      dictAdapter->m_dictImpl->disposeData( dictLValue );
    }

    void DictAdapter::DefaultAssign( void *_dictAdapter, void const *srcLValue, void *dstLValue )
    {
      DictAdapter const *dictAdapter = static_cast<DictAdapter const *>( _dictAdapter );
      dictAdapter->m_dictImpl->setData( srcLValue, dstLValue );
    }

    RC::ConstHandle<ComparableAdapter> DictAdapter::getKeyAdapter() const
    {
      return m_keyAdapter;
    }
    
    RC::ConstHandle<Adapter> DictAdapter::getValueAdapter() const
    {
      return m_valueAdapter;
    }
    
    llvm::StructType *DictAdapter::getLLVMBitsType( RC::Handle<Context> const &context ) const
    {
      return static_cast<llvm::StructType *>( static_cast<llvm::PointerType *>( llvmRawType( context ) )->getElementType() );
    }
    
    llvm::PointerType *DictAdapter::getLLVMBucketPtrType( RC::Handle<Context> const &context ) const
    {
      return static_cast<llvm::PointerType *>( getLLVMBitsType(context )->getTypeAtIndex( 5 ) );
    }
    
    llvm::StructType *DictAdapter::getLLVMBucketType( RC::Handle<Context> const &context ) const
    {
      return static_cast<llvm::StructType *>( getLLVMBucketPtrType( context )->getElementType() );
    }
    
    llvm::PointerType *DictAdapter::getLLVMNodePtrType( RC::Handle<Context> const &context ) const
    {
      return static_cast<llvm::PointerType *>( getLLVMBitsType(context )->getTypeAtIndex( 3 ) );
    }
    
    llvm::StructType *DictAdapter::getLLVMNodeType( RC::Handle<Context> const &context ) const
    {
      return static_cast<llvm::StructType *>( getLLVMNodePtrType( context )->getElementType() );
    }
  }
}
