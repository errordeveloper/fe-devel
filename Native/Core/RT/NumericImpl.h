/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_NUMERIC_IMPL_H
#define _FABRIC_RT_NUMERIC_IMPL_H

#include <Fabric/Core/RT/SimpleImpl.h>

namespace Fabric
{
  namespace RT
  {
    class NumericImpl : public SimpleImpl
    {
    public:
      REPORT_RC_LEAKS
    
      // NumericImpl

      virtual bool isInteger() const { return false; }
      virtual bool isFloat() const { return false; }
    
    protected:
    
      NumericImpl()
      {
      }
    };
  }
}

#endif //_FABRIC_RT_NUMERIC_IMPL_H
