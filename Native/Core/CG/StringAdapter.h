/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_STRING_ADAPTER_H
#define _FABRIC_CG_STRING_ADAPTER_H

#include <Fabric/Core/CG/Adapter.h>

#include <string.h>

namespace Fabric
{
  namespace RT
  {
    class Desc;
    class StringDesc;
  };
  
  namespace CG
  {
    class StringAdapter : public Adapter
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
      
      // ComparableAdapter
      
      virtual llvm::Value *llvmHash( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const;
      virtual llvm::Value *llvmCompare( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lhsRValue, llvm::Value *rhsRValue ) const;

      // StringAdapter

      void llvmReport( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *stringRValue ) const;

      llvm::Value *llvmCallLength( BasicBlockBuilder &basicBlockBuilder, llvm::Value *stringRValue ) const;
      void llvmCallCast( BasicBlockBuilder &basicBlockBuilder, RC::ConstHandle<Adapter> const &adapter, llvm::Value *srcLValue, llvm::Value *dstLValue ) const;
      void llvmCallConcat( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lhsRValue, llvm::Value *rhsRValue, llvm::Value *dstLValue ) const;

    protected:
      
      StringAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::StringDesc> const &stringDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;

      llvm::Type const *getLLVMImplType( RC::Handle<Context> const &context ) const;

      void llvmRetain( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const;
      void llvmRelease( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const;
      
    private:
      
      static void Append( void *dstLValue, void const *srcRValue );
      static void Cast( Adapter const *adapter, void const *srcLValue, void *dstLValue );
      
      RC::ConstHandle<RT::StringDesc> m_stringDesc;
    };
  };
};

#endif //_FABRIC_CG_STRING_ADAPTER_H
