/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_FIXED_ARRAY_ADAPTER_H
#define _FABRIC_CG_FIXED_ARRAY_ADAPTER_H

#include <Fabric/Core/CG/ArrayAdapter.h>

namespace Fabric
{
  namespace RT
  {
    class FixedArrayDesc;
  };
  
  namespace CG
  {
    class FixedArrayAdapter : public ArrayAdapter
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
      
      // Adapter
      
      virtual void llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const;
      virtual void llvmStore( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const;
      virtual void llvmDisposeImpl( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const;

      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual llvm::Constant *llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual llvm::Constant *llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const;
      
      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;

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
    
      FixedArrayAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::FixedArrayDesc> const &fixedArrayDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;

    private:
    
      size_t m_length;
      RC::ConstHandle<RT::Desc> m_fixedArrayDesc;
      RC::ConstHandle<Adapter> m_memberAdapter;
      std::string m_codeName;
   };
  };
};

#endif //_FABRIC_CG_FIXED_ARRAY_ADAPTER_H
