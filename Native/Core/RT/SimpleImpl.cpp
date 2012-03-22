/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "SimpleImpl.h"

namespace Fabric
{
  namespace RT
  {
    void SimpleImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
    }
    
    bool SimpleImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      return getType() == impl->getType();
    }
  }
}
