/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_ARRAY_ADAPTER_H
#define _FABRIC_CG_ARRAY_ADAPTER_H

#include <Fabric/Core/CG/Adapter.h>

namespace Fabric
{
  namespace RT
  {
    class ArrayDesc;
  };
  
  namespace CG
  {
    class BasicBlockBuilder;
    class ConstStringAdapter;
    class Location;
    class SizeAdapter;
    class StringAdapter;
    
    class ArrayAdapter : public Adapter
    {
    public:
      REPORT_RC_LEAKS
      
      // Adapter
      
      virtual std::string toString( void const *data ) const;
    
      // ArrayAdapter
    
      RC::ConstHandle<Adapter> getMemberAdapter() const
      {
        return m_memberAdapter;
      }

      virtual llvm::Value *llvmConstIndexOp(
        CG::BasicBlockBuilder &basicBlockBuilder,
        llvm::Value *arrayRValue,
        llvm::Value *indexRValue,
        CG::Location const *location
        ) const = 0;
      virtual llvm::Value *llvmNonConstIndexOp(
        CG::BasicBlockBuilder &basicBlockBuilder,
        llvm::Value *arrayLValue,
        llvm::Value *indexRValue,
        CG::Location const *location
        ) const = 0;
      
    protected:
    
      ArrayAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::ArrayDesc> const &arrayDesc, Flags flags );

      void llvmThrowOutOfRangeException(
        BasicBlockBuilder &basicBlockBuilder,
        llvm::Value *itemDescRValue,
        llvm::Value *indexRValue,
        llvm::Value *sizeRValue,
        llvm::Value *errorDescRValue
        ) const;
        
      llvm::Value *llvmLocationConstStringRValue(
        BasicBlockBuilder &basicBlockBuilder,
        RC::ConstHandle<ConstStringAdapter> const &constStringAdapter,
        CG::Location const *location
        ) const;
        
    private:
    
      RC::ConstHandle<RT::ArrayDesc> m_arrayDesc;
      RC::ConstHandle<Adapter> m_memberAdapter;
    };
  };
};

#endif //_FABRIC_CG_ARRAY_ADAPTER_H
