/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_PRODUCER_H
#define _FABRIC_MR_PRODUCER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace MR
  {
    class Producer : public RC::Object
    {
    public:
        virtual void flush() = 0;
    };
  }
}

#endif //_FABRIC_MR_PRODUCER_H
