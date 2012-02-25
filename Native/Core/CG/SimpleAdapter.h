/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_SIMPLE_ADAPTER_H
#define _FABRIC_CG_SIMPLE_ADAPTER_H

#include <Fabric/Core/CG/ComparableAdapter.h>

namespace Fabric
{
  namespace CG
  {
    class SimpleAdapter : public ComparableAdapter
    {
    public:
      REPORT_RC_LEAKS
    
      // Adapter
    
      virtual void llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const;

    protected:
    
      SimpleAdapter(
        RC::ConstHandle<Manager> const &manager,
        RC::ConstHandle<RT::ComparableDesc> const &comparableDesc
        );
    };
  };
};

#endif //_FABRIC_CG_SIMPLE_ADAPTER_H
