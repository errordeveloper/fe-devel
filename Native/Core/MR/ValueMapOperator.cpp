/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/MR/ValueMapOperator.h>
#include <Fabric/Core/RT/Desc.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ValueMapOperator> ValueMapOperator::Create(
      void (*functionPtr)(...),
      size_t numParams,
      RC::ConstHandle<RT::Desc> const &inputDesc,
      RC::ConstHandle<RT::Desc> const &outputDesc,
      RC::ConstHandle<RT::Desc> const &sharedDesc
      )
    {
      RC::Handle<ValueMapOperator> result( new ValueMapOperator );
      result->init( functionPtr, numParams, inputDesc, outputDesc, sharedDesc );
      return result;
    }

    ValueMapOperator::ValueMapOperator()
    {
    }
  }
}
