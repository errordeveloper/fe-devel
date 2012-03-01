/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/CG/Context.h>

namespace Fabric
{
  namespace CG
  {
    RC::Handle<Context> Context::Create()
    {
      return new Context;
    }
    
    Context::Context()
    {
    }
  };
};
