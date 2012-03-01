/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_VALUE_PRODUCER_ADAPTER_H
#define _FABRIC_CG_VALUE_PRODUCER_ADAPTER_H

#include <Fabric/Core/CG/Adapter.h>

#include <string.h>

namespace Fabric
{
  namespace RT
  {
    class Desc;
    class ValueProducerDesc;
  };
  
  namespace CG
  {
    class ValueProducerAdapter : public Adapter
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      // Adapter
    
      virtual void llvmInit( BasicBlockBuilder &basicBlockBuilder, llvm::Value *value ) const;
      virtual void llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const;
      virtual void llvmDisposeImpl( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const;

      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual llvm::Constant *llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual llvm::Constant *llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const;
      
      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;
      virtual void *llvmResolveExternalFunction( std::string const &functionName ) const;
      
      virtual std::string toString( void const *data ) const;

      // ValueProducerAdapter

      RC::ConstHandle<Adapter> getValueAdapter() const;
      void llvmProduce( BasicBlockBuilder &basicBlockBuilder, llvm::Value *srcRValue, llvm::Value *dstLValue ) const;
      void llvmFlush( BasicBlockBuilder &basicBlockBuilder, llvm::Value *srcRValue ) const;

    protected:
      
      ValueProducerAdapter(
        RC::ConstHandle<Manager> const &manager,
        RC::ConstHandle<RT::ValueProducerDesc> const &valueProducerDesc
        );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;

      llvm::Type const *getLLVMImplType( RC::Handle<Context> const &context ) const;

      void llvmRetain( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const;
      void llvmRelease( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const;
      
    private:
      
      static void Retain( void const *rValue );
      static void Release( void const *rValue );
      static void DefaultAssign( void const *srcRValue, void *dstLValue );
      static void Produce( void const *valueProducerRValue, void *dstLValue );
      static void Flush( void const *valueProducerRValue );
      
      RC::ConstHandle<RT::ValueProducerDesc> m_valueProducerDesc;
      RC::ConstHandle<Adapter> m_valueAdapter;
    };
  };
};

#endif //_FABRIC_CG_VALUE_PRODUCER_ADAPTER_H
