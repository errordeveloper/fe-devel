/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_DICT_ADAPTER_H
#define _FABRIC_CG_DICT_ADAPTER_H

#include <Fabric/Core/CG/Adapter.h>

namespace llvm
{
  class StructType;
  class PointerType;
};

namespace Fabric
{
  namespace RT
  {
    class DictDesc;
    class DictImpl;
  };
  
  namespace AST
  {
    class ContainerLoop;
  };
  
  namespace CG
  {
    class ComparableAdapter;
    
    class DictAdapter : public Adapter
    {
      friend class Manager;
      friend class AST::ContainerLoop;
    
    public:
      REPORT_RC_LEAKS

      // Adapter
    
      virtual void llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const;
      virtual void llvmDisposeImpl( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const;

      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual llvm::Constant *llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual llvm::Constant *llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const;
      
      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;
      virtual void *llvmResolveExternalFunction( std::string const &functionName ) const;

      virtual std::string toString( void const *data ) const;

      // DictAdapter
      
      RC::ConstHandle<ComparableAdapter> getKeyAdapter() const;
      RC::ConstHandle<Adapter> getValueAdapter() const;

      virtual llvm::Value *llvmGetRValue(
        CG::BasicBlockBuilder &basicBlockBuilder,
        llvm::Value *dictRValue,
        llvm::Value *keyRValue
        ) const;
      virtual llvm::Value *llvmGetLValue(
        CG::BasicBlockBuilder &basicBlockBuilder,
        llvm::Value *dictLValue,
        llvm::Value *keyRValue
        ) const;

    protected:
    
      DictAdapter(
        RC::ConstHandle<Manager> const &manager,
        RC::ConstHandle<RT::DictDesc> const &dictDesc
        );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;

      llvm::Value *llvmCallSize( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const;
      
      static bool Has( void *_dictAdapter, void const *dictLValue, void const *keyLValue );
      static void const *GetRValue( void *_dictAdapter, void const *dictRValue, void const *keyRValue );
      static void *GetLValue( void *_dictAdapter, void *dictLValue, void const *keyRValue );
      static void Delete( void *_dictAdapter, void *dictLValue, void const *keyLValue );
      static void Clear( void *_dictAdapter, void *dictLValue );
      static void Dispose( void *_dictAdapter, void *dictLValue );
      static void DefaultAssign( void *_dictAdapter, void const *srcLValue, void *dstLValue );

      llvm::StructType const *getLLVMBitsType( RC::Handle<Context> const &context ) const;
      llvm::PointerType const *getLLVMBucketPtrType( RC::Handle<Context> const &context ) const;
      llvm::StructType const *getLLVMBucketType( RC::Handle<Context> const &context ) const;
      llvm::PointerType const *getLLVMNodePtrType( RC::Handle<Context> const &context ) const;
      llvm::StructType const *getLLVMNodeType( RC::Handle<Context> const &context ) const;

    private:
    
      RC::ConstHandle<RT::DictDesc> m_dictDesc;
      RC::ConstHandle<ComparableAdapter> m_keyAdapter;
      RC::ConstHandle<Adapter> m_valueAdapter;
      
      RT::DictImpl const *m_dictImpl;
   };
  };
};

#endif //_FABRIC_CG_DICT_ADAPTER_H
