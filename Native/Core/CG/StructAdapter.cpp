/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "StructAdapter.h"
#include "BooleanAdapter.h"
#include "StringAdapter.h"
#include "SizeAdapter.h"
#include "OpaqueAdapter.h"
#include "Manager.h"
#include "ModuleBuilder.h"
#include "ConstructorBuilder.h"
#include "MethodBuilder.h"
#include "InternalFunctionBuilder.h"
#include "BasicBlockBuilder.h"
#include <Fabric/Core/CG/Mangling.h>

#include <Fabric/Core/RT/StructDesc.h>

#include <llvm/Module.h>
#include <llvm/Function.h>

namespace Fabric
{
  namespace CG
  {
    StructAdapter::StructAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::StructDesc> const &structDesc )
      : Adapter( manager, structDesc, FL_PASS_BY_REFERENCE )
      , m_structDesc( structDesc )
      , m_isShallow( structDesc->isShallow() )
    {
      size_t numMembers = m_structDesc->getNumMembers();
      m_memberAdapters.reserve( numMembers );
      for ( size_t i=0; i<numMembers; ++i )
      {
        RC::ConstHandle<Adapter> memberAdapter = getAdapter( m_structDesc->getMemberInfo( i ).desc );
        m_memberAdapters.push_back( memberAdapter );
      }
    }
    
    llvm::Type const *StructAdapter::buildLLVMRawType( RC::Handle<Context> const &context ) const
    {
      std::vector<llvm::Type const *> memberLLVMTypes;
      memberLLVMTypes.reserve( m_memberAdapters.size() );
      for ( size_t i=0; i<m_memberAdapters.size(); ++i )
        memberLLVMTypes.push_back( m_memberAdapters[i]->llvmRawType( context ) );
      return llvm::StructType::get( context->getLLVMContext(), memberLLVMTypes, true );
    }

    bool StructAdapter::hasMember( std::string const &memberName ) const
    {
      return m_structDesc->hasMember( memberName );
    }

    size_t StructAdapter::getMemberIndex( std::string const &memberName ) const
    {
      return m_structDesc->getMemberIndex( memberName );
    }
    
    RT::StructMemberInfo const &StructAdapter::getMemberInfo( size_t memberIndex ) const
    {
      return m_structDesc->getMemberInfo( memberIndex );
    }
    
    RC::ConstHandle<Adapter> StructAdapter::getMemberAdapter( size_t memberIndex ) const
    {
      FABRIC_ASSERT( memberIndex < m_memberAdapters.size() );
      return m_memberAdapters[memberIndex];
    }
    
    void StructAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      if ( !m_isShallow )
      {
        InternalFunctionBuilder functionBuilder(
          basicBlockBuilder.getModuleBuilder(),
          0, "__" + getCodeName() + "__DefaultAssign",
          "dst", this, USAGE_LVALUE,
          "src", this, USAGE_RVALUE,
          0
          );
        basicBlockBuilder->CreateCall2( functionBuilder.getLLVMFunction(), dstLValue, srcRValue );
      }
      else basicBlockBuilder->CreateStore( basicBlockBuilder->CreateLoad( srcRValue ), dstLValue );
    }

    void StructAdapter::llvmDisposeImpl( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const
    {
      for ( size_t i=0; i<m_memberAdapters.size(); ++i )
      {
        llvm::Value *memberLValue = basicBlockBuilder->CreateStructGEP( lValue, i );
        m_memberAdapters[i]->llvmDispose( basicBlockBuilder, memberLValue );
      }
    }

    void StructAdapter::llvmCompileToModule( ModuleBuilder &moduleBuilder ) const
    {
      if ( moduleBuilder.haveCompiledToModule( getCodeName() ) )
        return;
        
      RC::Handle<Context> context = moduleBuilder.getContext();
      
      RC::ConstHandle<StringAdapter> stringAdapter = getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<SizeAdapter> sizeAdapter = getManager()->getSizeAdapter();
      sizeAdapter->llvmCompileToModule( moduleBuilder );
      RC::ConstHandle<OpaqueAdapter> dataAdapter = getManager()->getDataAdapter();
      dataAdapter->llvmCompileToModule( moduleBuilder );
      for ( MemberAdaptorVector::const_iterator it=m_memberAdapters.begin(); it!=m_memberAdapters.end(); ++it )
        (*it)->llvmCompileToModule( moduleBuilder );
      
      moduleBuilder->addTypeName( getCodeName(), llvmRawType( context ) );
      
      static const bool buildFunctions = true;
      
      if ( !m_isShallow )
      {
        {
          InternalFunctionBuilder functionBuilder(
            moduleBuilder,
            0, "__" + getCodeName() + "__DefaultAssign",
            "dst", this, USAGE_LVALUE,
            "src", this, USAGE_RVALUE,
            0
            );
          if ( buildFunctions )
          {
            llvm::Value *dstLValue = functionBuilder[0];
            llvm::Value *srcRValue = functionBuilder[1];
            BasicBlockBuilder basicBlockBuilder( functionBuilder );
            basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
            for ( size_t i=0; i<getNumMembers(); ++i )
            {
              RC::ConstHandle<Adapter> const &memberAdapter = m_memberAdapters[i];
              llvm::Value *dstMemberLValue = basicBlockBuilder->CreateConstGEP2_32( dstLValue, 0, i );
              llvm::Value *srcMemberLValue = basicBlockBuilder->CreateConstGEP2_32( srcRValue, 0, i );
              llvm::Value *srcMemberRValue = memberAdapter->llvmLValueToRValue( basicBlockBuilder, srcMemberLValue );
              memberAdapter->llvmAssign( basicBlockBuilder, dstMemberLValue, srcMemberRValue );
            }
            basicBlockBuilder->CreateRetVoid();
          }
        }
      }
      
      {
        ConstructorBuilder functionBuilder( moduleBuilder, stringAdapter, this );
        if ( buildFunctions )
        {
          llvm::Value *stringLValue = functionBuilder[0];
          llvm::Value *structRValue = functionBuilder[1];
          BasicBlockBuilder basicBlockBuilder( functionBuilder );
          basicBlockBuilder->SetInsertPoint( functionBuilder.createBasicBlock( "entry" ) );
          llvm::Value *structLValue = llvmRValueToLValue( basicBlockBuilder, structRValue );
          stringAdapter->llvmCallCast( basicBlockBuilder, this, structLValue, stringLValue );
          basicBlockBuilder->CreateRetVoid();
        }
      }
      
      if ( getDesc()->isShallow() )
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
      }
    }
   
    std::string StructAdapter::toString( void const *data ) const
    {
      size_t numMembers = m_memberAdapters.size();
      std::string result = "{";
      for ( size_t i=0; i<numMembers; ++i )
      {
        if ( i > 0 )
          result += ",";
        result += m_structDesc->getMemberInfo(i).name + ":" + m_memberAdapters[i]->toString( m_structDesc->getImmutableMemberData( data, i ) );
      }
      return result + "}";
    }
    
    llvm::Constant *StructAdapter::llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      std::vector<llvm::Constant *> memberDefaultRValues;
      for ( size_t i=0; i<m_memberAdapters.size(); ++i )
      {
        RC::ConstHandle<Adapter> const &memberAdapter = m_memberAdapters[i];
        memberDefaultRValues.push_back( memberAdapter->llvmDefaultValue( basicBlockBuilder ) );
      }
      return llvm::ConstantStruct::get( basicBlockBuilder.getContext()->getLLVMContext(), memberDefaultRValues, true );
    }
      
    llvm::Constant *StructAdapter::llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const
    {
      return llvmDefaultLValue( basicBlockBuilder );
    }

    llvm::Constant *StructAdapter::llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const
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
  };
};
