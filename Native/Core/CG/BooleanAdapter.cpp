/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "BooleanAdapter.h"
#include "ConstStringAdapter.h"
#include "StringAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "FunctionBuilder.h"
#include "BasicBlockBuilder.h"
#include "OverloadNames.h"

#include <Fabric/Core/RT/BooleanDesc.h>
#include <Fabric/Core/Util/Format.h>

namespace Fabric
{
  namespace CG
  {
    BooleanAdapter::BooleanAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::BooleanDesc> const &booleanDesc )
      : SimpleAdapter( manager, booleanDesc )
      , m_booleanDesc( booleanDesc )
    {
      setLLVMType( llvm::Type::getInt1Ty( manager->getLLVMContext() ) );
    }
    
    void BooleanAdapter::llvmPrepareModule( ModuleBuilder &moduleBuilder, bool buildFunctions ) const
    {
      if ( moduleBuilder.contains( getCodeName(), buildFunctions ) )
        return;
      
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      
      {
        std::string name = constructOverloadName( stringAdapter, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "stringLValue", stringAdapter, USAGE_LVALUE ) );
        params.push_back( FunctionParam( "booleanRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType(), params );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *booleanRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::BasicBlock *trueBB = functionBuilder.createBasicBlock( "true" );
          llvm::BasicBlock *falseBB = functionBuilder.createBasicBlock( "false" );
          llvm::BasicBlock *mergeBB = functionBuilder.createBasicBlock( "merge" );
          basicBlockBuilder->CreateCondBr( booleanRValue, trueBB, falseBB );
          
          basicBlockBuilder->SetInsertPoint( trueBB );
          RC::ConstHandle<ConstStringAdapter> trueConstStringAdapter = getManager()->getConstStringAdapter(4);
          ExprValue trueExprValue( trueConstStringAdapter, USAGE_RVALUE, trueConstStringAdapter->llvmConst( basicBlockBuilder, "true", 4 ) );
          llvm::Value *trueStringRValue = stringAdapter->llvmCast( basicBlockBuilder, trueExprValue );
          basicBlockBuilder->CreateBr( mergeBB );
          
          basicBlockBuilder->SetInsertPoint( falseBB );
          RC::ConstHandle<ConstStringAdapter> falseConstStringAdapter = getManager()->getConstStringAdapter(5);
          ExprValue falseExprValue( falseConstStringAdapter, USAGE_RVALUE, falseConstStringAdapter->llvmConst( basicBlockBuilder, "false", 5 ) );
          llvm::Value *falseStringRValue = stringAdapter->llvmCast( basicBlockBuilder, falseExprValue );
          basicBlockBuilder->CreateBr( mergeBB );
          
          basicBlockBuilder->SetInsertPoint( mergeBB );
          llvm::PHINode *stringRValue = basicBlockBuilder->CreatePHI( stringAdapter->llvmRType(), "stringRValue" );
          stringRValue->addIncoming( trueStringRValue, trueBB );
          stringRValue->addIncoming( falseStringRValue, falseBB );
          stringAdapter->llvmAssign( basicBlockBuilder, stringLValue, stringRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      /*
      // [pzion 20110202] It's unclear that we want booleans to automatically cast to numerics
      
      {
        std::string name = constructOverloadName( byteDesc, ExprType( this, USAGE_RVALUE ) );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "booleanRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( byteDesc, USAGE_RVALUE ), params );
        llvm::Value *booleanRValue = functionBuilder[0];
        BasicBlockBuilder basicBlockBuilder( functionBuilder );
        basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
        llvm::Value *byteRValue = basicBlockBuilder->CreateZExt( booleanRValue, llvmRType() );
        basicBlockBuilder->CreateRet( byteRValue );
      }

      {
        std::string name = constructOverloadName( integerDesc, ExprType( this, USAGE_RVALUE ) );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "booleanRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( integerDesc, USAGE_RVALUE ), params );
        llvm::Value *booleanRValue = functionBuilder[0];
        BasicBlockBuilder basicBlockBuilder( functionBuilder );
        basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
        llvm::Value *integerRValue = basicBlockBuilder->CreateZExt( booleanRValue, llvmRType() );
        basicBlockBuilder->CreateRet( integerRValue );
      }
      
      {
        std::string name = constructOverloadName( scalarDesc, ExprType( this, USAGE_RVALUE ) );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "booleanRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( scalarDesc, USAGE_RVALUE ), params );
        llvm::Value *booleanRValue = functionBuilder[0];
        BasicBlockBuilder basicBlockBuilder( functionBuilder );
        basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
        llvm::Value *scalarRValue = basicBlockBuilder->CreateUIToFP( booleanRValue, llvmRType() );
        basicBlockBuilder->CreateRet( scalarRValue );
      }
      */
      
      {
        std::string name = uniOpOverloadName( UNI_OP_BIT_NOT, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "rValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( this, USAGE_RVALUE ), params );
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
        std::string name = binOpOverloadName( BIN_OP_BIT_OR, this, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "lhsRValue", this, USAGE_RVALUE ) );
        params.push_back( FunctionParam( "rhsRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( this, USAGE_RVALUE ), params );
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
        std::string name = binOpOverloadName( BIN_OP_BIT_AND, this, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "lhsRValue", this, USAGE_RVALUE ) );
        params.push_back( FunctionParam( "rhsRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( this, USAGE_RVALUE ), params );
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
        std::string name = binOpOverloadName( BIN_OP_BIT_XOR, this, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "lhsRValue", this, USAGE_RVALUE ) );
        params.push_back( FunctionParam( "rhsRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( this, USAGE_RVALUE ), params );
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
        std::string name = binOpOverloadName( BIN_OP_EQ, this, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "lhsRValue", this, USAGE_RVALUE ) );
        params.push_back( FunctionParam( "rhsRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( this, USAGE_RVALUE ), params );
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
        std::string name = binOpOverloadName( BIN_OP_NE, this, this );
        std::vector< FunctionParam > params;
        params.push_back( FunctionParam( "lhsRValue", this, USAGE_RVALUE ) );
        params.push_back( FunctionParam( "rhsRValue", this, USAGE_RVALUE ) );
        FunctionBuilder functionBuilder( moduleBuilder, name, ExprType( this, USAGE_RVALUE ), params );
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
    }

    llvm::Constant *BooleanAdapter::llvmConst( bool value ) const
    {
      return llvm::ConstantInt::get( llvmRType(), value, false );
    }
    
    std::string BooleanAdapter::toString( void const *data ) const
    {
      return _( m_booleanDesc->getValue(data) );
    }
  };
};
