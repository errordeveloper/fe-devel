/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/ProducerImpl.h>

namespace Fabric
{
  namespace RT
  {
    ProducerImpl::ProducerImpl()
    {
    }

    void ProducerImpl::initialize(
      std::string const &codeName,
      ImplType type,
      size_t allocSize
      )
    {
      Impl::initialize(
        codeName,
        type,
        allocSize,
        0
        );
    }
  }
}
