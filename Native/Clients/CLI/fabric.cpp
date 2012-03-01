/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Clients/CLI/ClientFactory.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Core/Build.h>

#include <time.h>

namespace Fabric
{
  namespace CLI
  {
    extern "C" FABRIC_CLI_EXPORT void init( v8::Handle<v8::Object> target )
    {
      FABRIC_LOG( "%s version %s", Fabric::buildName, Fabric::buildFullVersion );

      v8::HandleScope handleScope;
      target->Set( v8::String::New("createClient"), CreateClientV8Function() );
    }
  }
}
