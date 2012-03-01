/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "SimpleAdapter.h"
#include "Context.h"
#include "Manager.h"
#include "Context.h"
#include "BasicBlockBuilder.h"

#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/RT/Impl.h>

namespace Fabric
{
  namespace CG
  {
    SimpleAdapter::SimpleAdapter(
      RC::ConstHandle<Manager> const &manager,
      RC::ConstHandle<RT::ComparableDesc> const &desc
      )
      : ComparableAdapter( manager, desc, 0 )
    {
    }

    void SimpleAdapter::llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const
    {
      basicBlockBuilder->CreateStore( srcRValue, dstLValue );
    }
  };
};
