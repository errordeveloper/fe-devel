#include "FloatAdapter.h"
#include "BooleanAdapter.h"
#include "ByteAdapter.h"
#include "IntegerAdapter.h"
#include "SizeAdapter.h"
#include "StringAdapter.h"
#include "OpaqueAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "BinOpBuilder.h"
#include "UniOpBuilder.h"
#include "BasicBlockBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/RT/FloatDesc.h>
#include <Fabric/Core/RT/FloatImpl.h>
#include <Fabric/Base/Util/Format.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Intrinsics.h>
#include <llvm/Type.h>

namespace Fabric
{
  namespace CG
  {
    FloatAdapter::FloatAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::FloatDesc> const &floatDesc )
      : SimpleAdapter( manager, floatDesc )
      , m_floatDesc( floatDesc )
    {
    }
    
    llvm::Type const *FloatAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      llvm::Type const *result = 0;
      switch ( m_floatDesc->getAllocSize() )
      {
        case 4:
          result = llvm::Type::getFloatTy( context->getLLVMContext() );
          break;
        case 8:
          result = llvm::Type::getDoubleTy( context->getLLVMContext() );
          break;
        default:
          FABRIC_ASSERT( false );
          result = 0;
          break;
      }
      return result;
    }

    void FloatAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
      
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      RC::ConstHandle<BooleanAdapter> booleanAdapter = getManager()->getBooleanAdapter();
      booleanAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<ByteAdapter> byteAdapter = getManager()->getByteAdapter();
      byteAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<IntegerAdapter> integerAdapter = getManager()->getIntegerAdapter();
      integerAdapter->llvmCompileToModule( moduleBuilder );
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
          llvm::Value *scalarRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *booleanRValue = basicBlockBuilder->CreateFCmpONE( scalarRValue, llvmConst( context, 0.0 ) );
          booleanAdapter->llvmAssign( basicBlockBuilder, booleanLValue, booleanRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, byteAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *byteLValue = functionBuilder[0];
          llvm::Value *scalarRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *byteRValue = basicBlockBuilder->CreateFPToUI( scalarRValue, byteAdapter->llvmRType( context ) );
          byteAdapter->llvmAssign( basicBlockBuilder, byteLValue, byteRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, integerAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *integerLValue = functionBuilder[0];
          llvm::Value *scalarRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *integerRValue = basicBlockBuilder->CreateFPToSI( scalarRValue, integerAdapter->llvmRType( context ) );
          integerAdapter->llvmAssign( basicBlockBuilder, integerLValue, integerRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, sizeAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *sizeLValue = functionBuilder[0];
          llvm::Value *scalarRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *sizeRValue = basicBlockBuilder->CreateFPToUI( scalarRValue, sizeAdapter->llvmRType( context ) );
          sizeAdapter->llvmAssign( basicBlockBuilder, sizeLValue, sizeRValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      if ( m_floatDesc->getAllocSize() != 4 )
      {
        RC::ConstHandle<FloatAdapter> fp32Adapter = getManager()->getFP32Adapter();
        ConstructorBuilder functionBuilder( moduleBuilder, fp32Adapter, this, ConstructorBuilder::MedHighCost );
        if ( buildFunctions )
        {
          llvm::Value *fp32LValue = functionBuilder[0];
          llvm::Value *scalarRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *fp32RValue = basicBlockBuilder->CreateFPTrunc( scalarRValue, fp32Adapter->llvmRType( context ) );
          fp32Adapter->llvmAssign( basicBlockBuilder, fp32LValue, fp32RValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      if ( m_floatDesc->getAllocSize() != 8 )
      {
        RC::ConstHandle<FloatAdapter> fp64Adapter = getManager()->getFP64Adapter();
        ConstructorBuilder functionBuilder( moduleBuilder, fp64Adapter, this, ConstructorBuilder::LowCost );
        if ( buildFunctions )
        {
          llvm::Value *fp64LValue = functionBuilder[0];
          llvm::Value *scalarRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *fp64RValue = basicBlockBuilder->CreateFPExt( scalarRValue, fp64Adapter->llvmRType( context ) );
          fp64Adapter->llvmAssign( basicBlockBuilder, fp64LValue, fp64RValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, stringAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *scalarRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *scalarLValue = llvmRValueToLValue( basicBlockBuilder, scalarRValue );
          stringAdapter->llvmCallCast( basicBlockBuilder, this, scalarLValue, stringLValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      {
        UniOpBuilder functionBuilder( moduleBuilder, this, UNI_OP_POS, this, USAGE_RVALUE );
        if ( buildFunctions )
        {
          llvm::Value *rValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          basicBlockBuilder->CreateRet( rValue );
        }
      }
      
      {
        UniOpBuilder functionBuilder( moduleBuilder, this, UNI_OP_NEG, this, USAGE_RVALUE );
        if ( buildFunctions )
        {
          llvm::Value *rValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *negRValue = basicBlockBuilder->CreateFNeg( rValue );
          basicBlockBuilder->CreateRet( negRValue );
        }
      }
      
      {
        FunctionBuilder functionBuilder( moduleBuilder, this, "sin", "x", this, USAGE_RVALUE );
        if ( buildFunctions )
        {
          llvm::Value *x = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          static const size_t numIntrinsicTypes = 1;
          llvm::Type const *intrinsicTypes[numIntrinsicTypes] =
          {
            llvmRType( context )
          };
          llvm::Function *intrinsic = llvm::Intrinsic::getDeclaration( moduleBuilder, llvm::Intrinsic::sin, intrinsicTypes, numIntrinsicTypes );
          FABRIC_ASSERT( intrinsic );
          basicBlockBuilder->CreateRet( basicBlockBuilder->CreateCall( intrinsic, x ) );
        }
      }

      {
        FunctionBuilder functionBuilder( moduleBuilder, this, "cos", "x", this, USAGE_RVALUE );
        if ( buildFunctions )
        {
          llvm::Value *x = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          static const size_t numIntrinsicTypes = 1;
          llvm::Type const *intrinsicTypes[numIntrinsicTypes] =
          {
            llvmRType( context )
          };
          llvm::Function *intrinsic = llvm::Intrinsic::getDeclaration( moduleBuilder, llvm::Intrinsic::cos, intrinsicTypes, numIntrinsicTypes );
          FABRIC_ASSERT( intrinsic );
          basicBlockBuilder->CreateRet( basicBlockBuilder->CreateCall( intrinsic, x ) );
        }
      }

      FunctionBuilder( moduleBuilder, this, "tan", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      FunctionBuilder( moduleBuilder, this, "log10", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      FunctionBuilder( moduleBuilder, this, "log", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      FunctionBuilder( moduleBuilder, this, "exp", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      FunctionBuilder( moduleBuilder, this, "asin", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      FunctionBuilder( moduleBuilder, this, "acos", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      FunctionBuilder( moduleBuilder, this, "atan", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      FunctionBuilder( moduleBuilder, this, "atan2", "y", this, USAGE_RVALUE, "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );

      {
        FunctionBuilder functionBuilder( moduleBuilder, this, "sqrt", "x", this, USAGE_RVALUE );
        if ( buildFunctions )
        {
          llvm::Value *x = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          static const size_t numIntrinsicTypes = 1;
          llvm::Type const *intrinsicTypes[numIntrinsicTypes] =
          {
            llvmRType( context )
          };
          llvm::Function *intrinsic = llvm::Intrinsic::getDeclaration( moduleBuilder, llvm::Intrinsic::sqrt, intrinsicTypes, numIntrinsicTypes );
          FABRIC_ASSERT( intrinsic );
          basicBlockBuilder->CreateRet( basicBlockBuilder->CreateCall( intrinsic, x ) );
        }
      }

      {
        FunctionBuilder functionBuilder( moduleBuilder, this, "abs", "x", this, USAGE_RVALUE );
        if ( buildFunctions )
        {
          llvm::Type const *integerTypeOfSameWidth = 0;
          switch ( m_floatDesc->getAllocSize() )
          {
            case 4:
              integerTypeOfSameWidth = llvm::Type::getInt32Ty( context->getLLVMContext() );
              break;
            case 8:
              integerTypeOfSameWidth = llvm::Type::getInt64Ty( context->getLLVMContext() );
              break;
            default:
              FABRIC_ASSERT( false );
              integerTypeOfSameWidth = 0;
              break;
          }
          
          llvm::Value *x = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *xAsInt = basicBlockBuilder->CreateBitCast( x, integerTypeOfSameWidth );
          llvm::Value *zero = llvm::ConstantInt::get( integerTypeOfSameWidth, 0 );
          llvm::Value *notZero = basicBlockBuilder->CreateNot( zero );
          llvm::Value *mask = basicBlockBuilder->CreateLShr( notZero, llvm::ConstantInt::get( integerTypeOfSameWidth, 1 ) );
          llvm::Value *maskedXAsInt = basicBlockBuilder->CreateAnd( xAsInt, mask );
          llvm::Value *maskedX = basicBlockBuilder->CreateBitCast( maskedXAsInt, llvmRType( context ) );
          basicBlockBuilder->CreateRet( maskedX );
        }
      }

      FunctionBuilder( moduleBuilder, this, "round", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      FunctionBuilder( moduleBuilder, this, "ceil", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      FunctionBuilder( moduleBuilder, this, "floor", "x", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );

      FunctionBuilder( moduleBuilder, this, "pow", "x", this, USAGE_RVALUE, "y", this, USAGE_RVALUE, FunctionBuilder::ImportSymbol );
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_ADD, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateFAdd( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_SUB, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateFSub( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_MUL, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateFMul( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_DIV, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateFDiv( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
        }
      }
      
      {
        BinOpBuilder functionBuilder( moduleBuilder, this, BIN_OP_MOD, this, this );
        if ( buildFunctions )
        {
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue = basicBlockBuilder->CreateFRem( lhsRValue, rhsRValue );
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
          llvm::Value *resultRValue = basicBlockBuilder->CreateFCmpOEQ( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
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
          llvm::Value *resultRValue = basicBlockBuilder->CreateFCmpONE( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
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
          llvm::Value *resultRValue = basicBlockBuilder->CreateFCmpOGT( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
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
          llvm::Value *resultRValue = basicBlockBuilder->CreateFCmpOGE( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
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
          llvm::Value *resultRValue = basicBlockBuilder->CreateFCmpOLT( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
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
          llvm::Value *resultRValue = basicBlockBuilder->CreateFCmpOLE( lhsRValue, rhsRValue );
          basicBlockBuilder->CreateRet( resultRValue );
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
          llvm::Value *dataSizeRValue = llvm::ConstantInt::get( sizeAdapter->llvmRType( context ), getDesc()->getAllocSize() );
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
     
      {
        MethodBuilder functionBuilder(
          moduleBuilder, sizeAdapter,
          this, USAGE_RVALUE,
          "hash"
          );
        if ( buildFunctions )
        {
          llvm::Value *rValue = functionBuilder[0];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *resultRValue;
          switch ( m_floatDesc->getAllocSize() )
          {
            case 4:
            {
#if defined(FABRIC_ARCH_32BIT)
              resultRValue = basicBlockBuilder->CreateBitCast(
                rValue,
                sizeAdapter->llvmRType( context )
                );
#elif defined(FABRIC_ARCH_64BIT)
              resultRValue = basicBlockBuilder->CreateZExt(
                basicBlockBuilder->CreateBitCast(
                  rValue,
                  llvm::Type::getInt32Ty( context->getLLVMContext() )
                  ),
                sizeAdapter->llvmRType( context )
                );
#else
# error "Unsupported FABRIC_ARCH_..."
#endif
            }
            break;
            
            case 8:
            {
#if defined(FABRIC_ARCH_32BIT)
              resultRValue = basicBlockBuilder->CreateXor(
                basicBlockBuilder->CreateTrunc(
                  basicBlockBuilder->CreateBitCast(
                    rValue,
                    llvm::Type::getInt64Ty( context->getLLVMContext() )
                    ),
                  sizeAdapter->llvmRType( context )
                  ),
                basicBlockBuilder->CreateTrunc(
                  basicBlockBuilder->CreateLShr(
                    basicBlockBuilder->CreateBitCast(
                      rValue,
                      llvm::Type::getInt64Ty( context->getLLVMContext() )
                      ),
                    llvm::ConstantInt::get( llvm::Type::getInt64Ty( context->getLLVMContext() ), 32, false )
                  ),
                  sizeAdapter->llvmRType( context )
                  )
                );
#elif defined(FABRIC_ARCH_64BIT)
              resultRValue = basicBlockBuilder->CreateBitCast(
                rValue,
                sizeAdapter->llvmRType( context )
                );
#else
# error "Unsupported FABRIC_ARCH_..."
#endif
            }
            break;
          }
          basicBlockBuilder->CreateRet( resultRValue );
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
          llvm::Value *lhsRValue = functionBuilder[0];
          llvm::Value *rhsRValue = functionBuilder[1];
          
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          llvm::BasicBlock *entryBB = functionBuilder.createBasicBlock( "entry" );
          llvm::BasicBlock *ltBB = functionBuilder.createBasicBlock( "lt" );
          llvm::BasicBlock *geBB = functionBuilder.createBasicBlock( "ge" );
          llvm::BasicBlock *eqBB = functionBuilder.createBasicBlock( "eq" );
          llvm::BasicBlock *gtBB = functionBuilder.createBasicBlock( "gt" );
          
          basicBlockBuilder->SetInsertPoint( entryBB );
          basicBlockBuilder->CreateCondBr( basicBlockBuilder->CreateFCmpOLT( lhsRValue, rhsRValue ), ltBB, geBB );
          
          basicBlockBuilder->SetInsertPoint( ltBB );
          basicBlockBuilder->CreateRet( integerAdapter->llvmConst( context, -1 ) );
          
          basicBlockBuilder->SetInsertPoint( geBB );
          basicBlockBuilder->CreateCondBr( basicBlockBuilder->CreateFCmpOEQ( lhsRValue, rhsRValue ), eqBB, gtBB );
          
          basicBlockBuilder->SetInsertPoint( eqBB );
          basicBlockBuilder->CreateRet( integerAdapter->llvmConst( context, 0 ) );
          
          basicBlockBuilder->SetInsertPoint( gtBB );
          basicBlockBuilder->CreateRet( integerAdapter->llvmConst( context, 1 ) );
        }
      }
    }
    
    llvm::Constant *FloatAdapter::llvmConst( RC::Handle<Context> const &context, double value ) const
    {
      return llvm::ConstantFP::get( llvmRawType( context ), value );
    }
    
    llvm::Constant *FloatAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      switch ( m_floatDesc->getAllocSize() )
      {
        case 4:
        {
          RC::ConstHandle<RT::FP32Impl> fp32Impl = RC::ConstHandle<RT::FP32Impl>::StaticCast( m_floatDesc->getImpl() );
          return llvmConst( basicBlockBuilder.getContext(), fp32Impl->getValue( fp32Impl->getDefaultData() ) );
        }
        break;
        
        case 8:
        {
          RC::ConstHandle<RT::FP64Impl> fp64Impl = RC::ConstHandle<RT::FP64Impl>::StaticCast( m_floatDesc->getImpl() );
          return llvmConst( basicBlockBuilder.getContext(), fp64Impl->getValue( fp64Impl->getDefaultData() ) );
        }
        break;
        
        default:
          FABRIC_ASSERT( false );
          return 0;
      }
    }
    
    std::string FloatAdapter::toString( void const *data ) const
    {
      return m_floatDesc->toString( data );
    }

    std::string FloatAdapter::internalFunctionNameForType( std::string const &externalFunctionName ) const
    {
      switch ( m_floatDesc->getAllocSize() )
      {
        case 4: return externalFunctionName;
        case 8: return externalFunctionName + "64";
        default:
          FABRIC_ASSERT( false );
          return "";
      }
    }

    std::string FloatAdapter::externalFunctionNameForType( std::string const &externalFunctionName ) const
    {
      switch ( m_floatDesc->getAllocSize() )
      {
        case 4: return "fp32_" + externalFunctionName;
        case 8: return "fp64_" + externalFunctionName;
        default:
          FABRIC_ASSERT( false );
          return "";
      }
    }
  };
};
