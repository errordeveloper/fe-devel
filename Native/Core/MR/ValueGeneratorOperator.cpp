/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueGeneratorOperator.h>
#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Core/RT/IntegerDesc.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ValueGeneratorOperator> ValueGeneratorOperator::Create(
      void (*functionPtr)(...),
      size_t numParams,
      RC::ConstHandle<RT::Desc> const &valueDesc,
      RC::ConstHandle<RT::Desc> const &sharedDesc
      )
    {
      RC::Handle<ValueGeneratorOperator> result( new ValueGeneratorOperator );
      result->init( functionPtr, numParams, valueDesc, sharedDesc );
      return result;
    }

    ValueGeneratorOperator::ValueGeneratorOperator()
    {
    }
  }
}
