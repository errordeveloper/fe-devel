/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayGeneratorOperator.h>
#include <Fabric/Core/RT/Desc.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ArrayGeneratorOperator> ArrayGeneratorOperator::Create(
      void (*functionPtr)(...),
      size_t numParams,
      RC::ConstHandle<RT::Desc> const &valueDesc,
      RC::ConstHandle<RT::Desc> const &sharedDesc
      )
    {
      RC::Handle<ArrayGeneratorOperator> result( new ArrayGeneratorOperator );
      result->init( functionPtr, numParams, valueDesc, sharedDesc );
      return result;
    }

    ArrayGeneratorOperator::ArrayGeneratorOperator()
    {
    }
  }
}
