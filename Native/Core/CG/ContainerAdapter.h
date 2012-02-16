/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_CG_CONTAINER_ADAPTER_H
#define _FABRIC_CG_CONTAINER_ADAPTER_H

#include <Fabric/Core/CG/Adapter.h>

#include <string.h>

namespace Fabric
{
  namespace RT
  {
    class Desc;
    class ContainerDesc;
  };
  
  namespace CG
  {
    class ContainerAdapter : public Adapter
    {
      friend class Manager;
      
    public:
    
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

    protected:
      
      ContainerAdapter(
        RC::ConstHandle<Manager> const &manager,
        RC::ConstHandle<RT::ContainerDesc> const &containerDesc
        );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;

      llvm::Type const *getLLVMImplType( RC::Handle<Context> const &context ) const;

      void llvmSetCount( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *containerLValue, llvm::Value *newSizeRValue ) const;
      llvm::Value *llvmGetCount( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *containerRValue ) const;

    private:

      RC::ConstHandle<RT::ContainerDesc> m_containerDesc;
    };
  };
};

#endif //_FABRIC_CG_CONTAINER_ADAPTER_H
