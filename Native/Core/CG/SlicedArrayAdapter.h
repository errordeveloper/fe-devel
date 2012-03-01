/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_SLICED_ARRAY_ADAPTER_H
#define _FABRIC_CG_SLICED_ARRAY_ADAPTER_H

#include <Fabric/Core/CG/ArrayAdapter.h>

namespace Fabric
{
  namespace RT
  {
    class SlicedArrayDesc;
    class SlicedArrayImpl;
  };
  
  namespace CG
  {
    class VariableArrayAdapter;
    
    class SlicedArrayAdapter : public ArrayAdapter
    {
      friend class Manager;
    
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

      // ArrayAdapter

      virtual llvm::Value *llvmConstIndexOp(
        CG::BasicBlockBuilder &basicBlockBuilder,
        llvm::Value *arrayRValue,
        llvm::Value *indexRValue,
        CG::Location const *location
        ) const;
      virtual llvm::Value *llvmNonConstIndexOp(
        CG::BasicBlockBuilder &basicBlockBuilder,
        llvm::Value *arrayLValue,
        llvm::Value *indexRValue,
        CG::Location const *location
        ) const;

    protected:
    
      SlicedArrayAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::SlicedArrayDesc> const &slicedArrayDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;

      static void DefaultAssign( SlicedArrayAdapter const *inst, void const *srcData, void *dstData );
      
    private:
    
      RC::ConstHandle<RT::SlicedArrayDesc> m_slicedArrayDesc;
      RC::ConstHandle<RT::SlicedArrayImpl> m_slicedArrayImpl;
      RC::ConstHandle<Adapter> m_memberAdapter;
      RC::ConstHandle<VariableArrayAdapter> m_variableArrayAdapter;
   };
  };
};

#endif //_FABRIC_CG_SLICED_ARRAY_ADAPTER_H
