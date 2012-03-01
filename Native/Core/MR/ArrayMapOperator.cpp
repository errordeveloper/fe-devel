/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayMapOperator.h>
#include <Fabric/Core/RT/Desc.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ArrayMapOperator> ArrayMapOperator::Create(
      void (*functionPtr)(...),
      size_t numParams,
      RC::ConstHandle<RT::Desc> const &inputDesc,
      RC::ConstHandle<RT::Desc> const &outputDesc,
      RC::ConstHandle<RT::Desc> const &sharedDesc
      )
    {
      RC::Handle<ArrayMapOperator> result( new ArrayMapOperator );
      result->init( functionPtr, numParams, inputDesc, outputDesc, sharedDesc );
      return result;
    }

    ArrayMapOperator::ArrayMapOperator()
    {
    }
  }
}
