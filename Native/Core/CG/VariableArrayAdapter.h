/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_VARIABLE_ARRAY_ADAPTER_H
#define _FABRIC_CG_VARIABLE_ARRAY_ADAPTER_H

#include <Fabric/Core/CG/ArrayAdapter.h>

namespace Fabric
{
  namespace RT
  {
    class VariableArrayDesc;
    class VariableArrayImpl;
  };
  
  namespace CG
  {
    class VariableArrayAdapter : public ArrayAdapter
    {
      friend class Manager;
      friend class SlicedArrayAdapter;
    
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

      // VariableArrayAdapter

      llvm::Value *llvmConstIndexOp_NoCheck( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayRValue, llvm::Value *indexRValue ) const;
      llvm::Value *llvmNonConstIndexOp_NoCheck( CG::BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayLValue, llvm::Value *indexRValue ) const;

    protected:
    
      VariableArrayAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::VariableArrayDesc> const &variableArrayDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;
      
    private:
    
      static void Append( VariableArrayAdapter const *inst, void *dstLValue, void const *srcRValue );
      static void Pop( VariableArrayAdapter const *inst, void *dst, void *result );
 
      void llvmCallPop( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayLValue, llvm::Value *memberLValue ) const;
      llvm::Value *llvmCallSize( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayRValue ) const;
      void llvmCallResize( BasicBlockBuilder &basicBlockBuilder, llvm::Value *arrayLValue, llvm::Value *newSize ) const;
      
      static const uint8_t AllocSizeIndex = 0;
      static const uint8_t SizeIndex = 1;
      static const uint8_t MemberDatasIndex = 2;
 
      RC::ConstHandle<RT::VariableArrayDesc> m_variableArrayDesc;
      RC::ConstHandle<RT::VariableArrayImpl> m_variableArrayImpl;
      RC::ConstHandle<Adapter> m_memberAdapter;
   };
  };
};

#endif //_FABRIC_CG_VARIABLE_ARRAY_ADAPTER_H
