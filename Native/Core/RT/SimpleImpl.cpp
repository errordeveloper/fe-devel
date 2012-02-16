/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "SimpleImpl.h"

namespace Fabric
{
  namespace RT
  {
    void SimpleImpl::disposeDatasImpl( void *data, size_t count, size_t stride ) const
    {
    }
    
    bool SimpleImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      return getType() == impl->getType();
    }
    
    bool SimpleImpl::isShallow() const
    {
      return true;
    }

    bool SimpleImpl::isNoAliasSafe() const
    {
      return true;
    }

    bool SimpleImpl::isExportable() const
    {
      return true;
    }
  }
}
