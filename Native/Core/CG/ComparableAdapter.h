/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_COMPARABLE_ADAPTER_H
#define _FABRIC_CG_COMPARABLE_ADAPTER_H

#include <Fabric/Core/CG/Adapter.h>

namespace llvm
{
  class Constant;
};

namespace Fabric
{
  namespace RT
  {
    class ComparableDesc;
  };
  
  namespace CG
  {
    class ComparableAdapter : public Adapter
    {
    public:
      REPORT_RC_LEAKS
      
      // ComparableAdapter
      
      llvm::Value *llvmHash( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const;
      llvm::Value *llvmCompare( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lhsRValue, llvm::Value *rhsRValue ) const;
    
    protected:

      ComparableAdapter(
        RC::ConstHandle<Manager> const &manager,
        RC::ConstHandle<RT::ComparableDesc> const &comparableDesc,
        Flags flags
        );
    };
  };
};

#endif //_FABRIC_CG_COMPARABLE_ADAPTER_H
