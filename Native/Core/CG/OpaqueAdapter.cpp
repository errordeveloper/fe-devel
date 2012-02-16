/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "OpaqueAdapter.h"
#include "BooleanAdapter.h"
#include "SizeAdapter.h"
#include "StringAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "BasicBlockBuilder.h"
#include "Scope.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/RT/OpaqueDesc.h>
#include <Fabric/Core/Util/Hex.h>

// For debugging, if contents of the opaque type are wanted
// when casting from string.
// #define FABRIC_PRINT_OPAQUE

namespace Fabric
{
  namespace CG
  {
    OpaqueAdapter::OpaqueAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::OpaqueDesc> const &opaqueDesc )
      : Adapter( manager, opaqueDesc, 0 )
      , m_opaqueDesc( opaqueDesc )
    {
    }
    
    llvm::Type const *OpaqueAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::Type const *result;
      size_t size = m_opaqueDesc->getAllocSize();
      if ( size == sizeof( void * ) )
        result = llvm::Type::getInt8PtrTy( context->getLLVMContext() );
      else
      {
        switch ( size )
        {
          case 1:
            result = llvm::Type::getInt8Ty( context->getLLVMContext() );
            break;
          case 2:
            result = llvm::Type::getInt16Ty( context->getLLVMContext() );
            break;
          case 4:
            result = llvm::Type::getInt32Ty( context->getLLVMContext() );
            break;
          case 8:
            result = llvm::Type::getInt64Ty( context->getLLVMContext() );
            break;
          default:
            FABRIC_ASSERT( false && "Opaque size must be 1, 2, 4 or 8" );
            result = 0;
            break;
        }
      }
      return result;
    }
    
    llvm::Constant *OpaqueAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      if ( getDesc()->getAllocSize() == sizeof( void * ) )
        return llvm::ConstantPointerNull::get( (llvm::PointerType const *)llvmRType( context ) );
      else 
        return llvm::ConstantInt::get( llvmRType( context ), 0 );
    }

    void OpaqueAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
      
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<OpaqueAdapter> dataAdapter = getManager()->getDataAdapter();
      dataAdapter->llvmCompileToModule( moduleBuilder );
      
      static const bool buildFunctions = true;
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, booleanAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *booleanLValue = functionBuilder[0];
          llvm::Value *opaqueRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *booleanRValue = basicBlockBuilder->CreateIsNotNull( opaqueRValue );
          booleanAdapter->llvmAssign( basicBlockBuilder, booleanLValue, booleanRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, stringAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *opaqueRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *opaqueLValue = llvmRValueToLValue( basicBlockBuilder, opaqueRValue );
          stringAdapter->llvmCallCast( basicBlockBuilder, this, opaqueLValue, stringLValue );
          basicBlockBuilder->CreateRetVoid();
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
          llvm::Value *dataSizeRValue = sizeAdapter->llvmConst( context, getDesc()->getAllocSize() );
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
    }
    
    std::string OpaqueAdapter::toString( void const *data ) const
    {
#if defined( FABRIC_PRINT_OPAQUE )
      return "<" + Util::hexBuf( getDesc()->getSize(), data ) + ">";
#else
      return "<Opaque>";
#endif
    }

    void OpaqueAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      basicBlockBuilder->CreateStore( srcRValue, dstLValue );
    }
  };
};
