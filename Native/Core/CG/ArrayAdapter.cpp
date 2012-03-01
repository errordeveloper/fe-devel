/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ArrayAdapter.h"
#include "Manager.h"
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/ConstStringAdapter.h>
#include <Fabric/Core/CG/ExprValue.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/Location.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/StringAdapter.h>
#include <Fabric/Core/RT/ArrayDesc.h>

#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>

namespace Fabric
{
  namespace CG
  {
    ArrayAdapter::ArrayAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::ArrayDesc> const &arrayDesc, Flags flags )
      : Adapter( manager, arrayDesc, flags )
      , m_arrayDesc( arrayDesc )
      , m_memberAdapter( manager->getAdapter( arrayDesc->getMemberDesc() ) )
    {
    }
    
    std::string ArrayAdapter::toString( void const *data ) const
    {
      size_t numMembers = m_arrayDesc->getNumMembers( data );
      std::string result = "[";
      for ( size_t i=0; i<numMembers; ++i )
      {
        if ( i > 0 )
          result += ",";
        result += m_memberAdapter->toString( m_arrayDesc->getImmutableMemberData( data, i ) );
      }
      return result + "]";
    }

    void ArrayAdapter::llvmThrowOutOfRangeException(
      BasicBlockBuilder &basicBlockBuilder,
      llvm::Value *itemDescRValue,
      llvm::Value *indexRValue,
      llvm::Value *sizeRValue,
      llvm::Value *errorDescRValue
      ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      RC::ConstHandle<SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
      RC::ConstHandle<StringAdapter> stringAdapter = basicBlockBuilder.getManager()->getStringAdapter();
      RC::ConstHandle<ConstStringAdapter> constStringAdapter = basicBlockBuilder.getManager()->getConstStringAdapter();

      std::vector<llvm::Type const *> argTypes;
      argTypes.push_back( constStringAdapter->llvmRType( context ) );
      argTypes.push_back( sizeAdapter->llvmRType( context ) );
      argTypes.push_back( sizeAdapter->llvmRType( context ) );
      argTypes.push_back( constStringAdapter->llvmRType( context ) );
      llvm::FunctionType const *funcType = llvm::FunctionType::get( basicBlockBuilder->getVoidTy(), argTypes, false );
      
      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, 0 /*llvm::Attribute::InlineHint | llvm::Attribute::NoUnwind*/ );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get( AWI, 1 );

      std::string name = "__ThrowOutOfRangeException";
      llvm::Function *func = llvm::cast<llvm::Function>( basicBlockBuilder.getModuleBuilder()->getFunction( name ) );
      if ( !func )
      {
        ModuleBuilder &mb = basicBlockBuilder.getModuleBuilder();
        
        func = llvm::cast<llvm::Function>( mb->getOrInsertFunction( name, funcType, attrListPtr ) ); 
        func->setLinkage( llvm::GlobalValue::PrivateLinkage );
        
        FunctionBuilder fb( mb, funcType, func );
        llvm::Argument *itemRValue = fb[0];
        itemRValue->setName( "itemRValue" );
        itemRValue->addAttr( llvm::Attribute::NoCapture );
        llvm::Argument *indexRValue = fb[1];
        indexRValue->setName( "indexRValue" );
        llvm::Argument *sizeRValue = fb[2];
        sizeRValue->setName( "sizeRValue" );
        llvm::Argument *errorDescRValue = fb[3];
        errorDescRValue->setName( "errorDescRValue" );
        errorDescRValue->addAttr( llvm::Attribute::NoCapture );
        
        BasicBlockBuilder bbb( fb );
        llvm::BasicBlock *entryBB = fb.createBasicBlock( "entry" );
        bbb->SetInsertPoint( entryBB );

        llvm::Value *errorMsg1PlusIndexPlusErrorMsg2PlusSizePlusErrorMsg3LValue;
        {
          Scope subScope( bbb.getScope() );
          BasicBlockBuilder subBasicBlockBuilder( bbb, subScope );

          llvm::BasicBlock *haveErrorDescBB = fb.createBasicBlock( "haveErrorDescBB" );
          llvm::BasicBlock *noErrorDescBB = fb.createBasicBlock( "noErrorDescBB" );
          llvm::BasicBlock *goBB = fb.createBasicBlock( "goBB" );

          subBasicBlockBuilder->CreateCondBr(
            subBasicBlockBuilder->CreateIsNotNull( errorDescRValue ),
            haveErrorDescBB,
            noErrorDescBB
            );
            
          subBasicBlockBuilder->SetInsertPoint( haveErrorDescBB );
          llvm::Value *haveErrorDescConstStringRValue = errorDescRValue;
          subBasicBlockBuilder->CreateBr( goBB );
            
          subBasicBlockBuilder->SetInsertPoint( noErrorDescBB );
          llvm::Value *noErrorDescConstStringRValue = constStringAdapter->llvmConst( subBasicBlockBuilder, "KL" );
          subBasicBlockBuilder->CreateBr( goBB );

          subBasicBlockBuilder->SetInsertPoint( goBB );
          llvm::PHINode *errorDescConstStringRValue = subBasicBlockBuilder->CreatePHI( haveErrorDescConstStringRValue->getType(), "errorDescConstStringRValue" );
          errorDescConstStringRValue->addIncoming( haveErrorDescConstStringRValue, haveErrorDescBB );
          errorDescConstStringRValue->addIncoming( noErrorDescConstStringRValue, noErrorDescBB );

          llvm::Value *errorMsg0RValue = stringAdapter->llvmCast(
            subBasicBlockBuilder,
            ExprValue( constStringAdapter, USAGE_RVALUE, context, errorDescConstStringRValue )
            );
          
          llvm::Value *errorMsgARValue = stringAdapter->llvmCast( subBasicBlockBuilder,
            ExprValue( constStringAdapter, USAGE_RVALUE, context, constStringAdapter->llvmConst( subBasicBlockBuilder, ": " ) )
            );
          
          llvm::Value *errorMsgALValue = stringAdapter->llvmAlloca( subBasicBlockBuilder, "errorMsgA" );
          stringAdapter->llvmInit( subBasicBlockBuilder, errorMsgALValue );
          stringAdapter->llvmCallConcat(
            subBasicBlockBuilder,
            errorMsg0RValue,
            errorMsgARValue,
            errorMsgALValue
            );
          
          llvm::Value *errorMsgBRValue = stringAdapter->llvmCast( subBasicBlockBuilder,
            ExprValue( constStringAdapter, USAGE_RVALUE, context, itemRValue )
            );
          
          llvm::Value *errorMsgBLValue = stringAdapter->llvmAlloca( subBasicBlockBuilder, "errorMsgB" );
          stringAdapter->llvmInit( subBasicBlockBuilder, errorMsgBLValue );
          stringAdapter->llvmCallConcat(
            subBasicBlockBuilder,
            stringAdapter->llvmLValueToRValue( subBasicBlockBuilder, errorMsgALValue ),
            errorMsgBRValue,
            errorMsgBLValue
            );
          stringAdapter->llvmDispose( subBasicBlockBuilder, errorMsgALValue );
          
          llvm::Value *errorMsgCRValue = stringAdapter->llvmCast( subBasicBlockBuilder,
            ExprValue( constStringAdapter, USAGE_RVALUE, context, constStringAdapter->llvmConst( subBasicBlockBuilder, " (" ) )
            );
          
          llvm::Value *errorMsgCLValue = stringAdapter->llvmAlloca( subBasicBlockBuilder, "errorMsgC" );
          stringAdapter->llvmInit( subBasicBlockBuilder, errorMsgCLValue );
          stringAdapter->llvmCallConcat(
            subBasicBlockBuilder,
            stringAdapter->llvmLValueToRValue( subBasicBlockBuilder, errorMsgBLValue ),
            errorMsgCRValue,
            errorMsgCLValue
            );
          stringAdapter->llvmDispose( subBasicBlockBuilder, errorMsgBLValue );
          
          llvm::Value *indexStringRValue = stringAdapter->llvmCast(
            subBasicBlockBuilder,
            ExprValue( sizeAdapter, USAGE_RVALUE, context, indexRValue )
            );
            
          llvm::Value *errorMsg1PlusIndexLValue = stringAdapter->llvmAlloca( subBasicBlockBuilder, "errorMsg1PlusIndex" );
          stringAdapter->llvmInit( subBasicBlockBuilder, errorMsg1PlusIndexLValue );
          stringAdapter->llvmCallConcat(
            subBasicBlockBuilder,
            stringAdapter->llvmLValueToRValue( subBasicBlockBuilder, errorMsgCLValue ),
            indexStringRValue,
            errorMsg1PlusIndexLValue
            );
          stringAdapter->llvmDispose( subBasicBlockBuilder, errorMsgCLValue );
          
          llvm::Value *errorMsg2RValue = stringAdapter->llvmCast(
            subBasicBlockBuilder,
            ExprValue( constStringAdapter, USAGE_RVALUE, context, constStringAdapter->llvmConst( subBasicBlockBuilder, ") out of range (" ) )
            );
            
          llvm::Value *errorMsg1PlusIndexPlusErrorMsg2LValue = stringAdapter->llvmAlloca( subBasicBlockBuilder, "errorMsg1PlusIndexPlusErrorMsg2" );
          stringAdapter->llvmInit( subBasicBlockBuilder, errorMsg1PlusIndexPlusErrorMsg2LValue );
          stringAdapter->llvmCallConcat(
            subBasicBlockBuilder,
            stringAdapter->llvmLValueToRValue( subBasicBlockBuilder, errorMsg1PlusIndexLValue ),
            errorMsg2RValue,
            errorMsg1PlusIndexPlusErrorMsg2LValue
            );
          stringAdapter->llvmDispose( subBasicBlockBuilder, errorMsg1PlusIndexLValue );

          llvm::Value *sizeStringRValue = stringAdapter->llvmCast(
            subBasicBlockBuilder,
            ExprValue( sizeAdapter, USAGE_RVALUE, context, sizeRValue )
            );
            
          llvm::Value *errorMsg1PlusIndexPlusErrorMsg2PlusSizeLValue = stringAdapter->llvmAlloca( subBasicBlockBuilder, "errorMsg1PlusIndexPlusErrorMsg2PlusSize" );
          stringAdapter->llvmInit( subBasicBlockBuilder, errorMsg1PlusIndexPlusErrorMsg2PlusSizeLValue );
          stringAdapter->llvmCallConcat(
            subBasicBlockBuilder,
            stringAdapter->llvmLValueToRValue( subBasicBlockBuilder, errorMsg1PlusIndexPlusErrorMsg2LValue ),
            sizeStringRValue,
            errorMsg1PlusIndexPlusErrorMsg2PlusSizeLValue
            );
          stringAdapter->llvmDispose( subBasicBlockBuilder, errorMsg1PlusIndexPlusErrorMsg2LValue );

          llvm::Value *errorMsg3RValue = stringAdapter->llvmCast(
            subBasicBlockBuilder,
            ExprValue( constStringAdapter, USAGE_RVALUE, context, constStringAdapter->llvmConst( subBasicBlockBuilder, ")" ) )
            );
            
          errorMsg1PlusIndexPlusErrorMsg2PlusSizePlusErrorMsg3LValue = stringAdapter->llvmAlloca( subBasicBlockBuilder, "errorMsg1PlusIndexPlusErrorMsg2PlusSizePlusErrorMsg3" );
          stringAdapter->llvmInit( subBasicBlockBuilder, errorMsg1PlusIndexPlusErrorMsg2PlusSizePlusErrorMsg3LValue );
          stringAdapter->llvmCallConcat(
            subBasicBlockBuilder,
            stringAdapter->llvmLValueToRValue( subBasicBlockBuilder, errorMsg1PlusIndexPlusErrorMsg2PlusSizeLValue ),
            errorMsg3RValue,
            errorMsg1PlusIndexPlusErrorMsg2PlusSizePlusErrorMsg3LValue
            );
          stringAdapter->llvmDispose( subBasicBlockBuilder, errorMsg1PlusIndexPlusErrorMsg2PlusSizeLValue );
          
          subScope.llvmUnwind( subBasicBlockBuilder );
        }
        llvmThrowException( bbb, errorMsg1PlusIndexPlusErrorMsg2PlusSizePlusErrorMsg3LValue );
        bbb->CreateRetVoid();
      }
      
      std::vector<llvm::Value *> args;
      args.push_back( itemDescRValue );
      args.push_back( indexRValue );
      args.push_back( sizeRValue );
      args.push_back( errorDescRValue );
      basicBlockBuilder->CreateCall( func, args.begin(), args.end() );
    }

    llvm::Value *ArrayAdapter::llvmLocationConstStringRValue(
      BasicBlockBuilder &basicBlockBuilder,
      RC::ConstHandle<ConstStringAdapter> const &constStringAdapter,
      CG::Location const *location
      ) const
    {
      if ( location )
      {
        RC::ConstHandle<RC::String> filename = location->getFilename();
        std::string locationString = (filename? filename->stdString(): std::string("(unknown)")) + ":" + _(location->getLine()) + ":" + _(location->getColumn());
        return constStringAdapter->llvmConst( basicBlockBuilder, locationString );
      }
      else return constStringAdapter->llvmConst( basicBlockBuilder, "KL" );
    }
  };
};
