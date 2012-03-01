/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_V8_CLIENT_FACTORY_H
#define _FABRIC_V8_CLIENT_FACTORY_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <v8.h>

namespace Fabric
{
  namespace CLI
  {
    v8::Handle<v8::Value> CreateClientV8Function();
  };
};

#endif //_FABRIC_V8_CLIENT_FACTORY_H
