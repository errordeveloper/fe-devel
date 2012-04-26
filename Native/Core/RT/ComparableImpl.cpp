/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ComparableImpl.h"

namespace Fabric
{
  namespace RT
  {
    ComparableImpl::ComparableImpl()
    {
    }
    
    bool ComparableImpl::equalsData( void const *lhs, void const *rhs ) const
    {
      return compare( lhs, rhs ) == 0;
    }
  }
}
