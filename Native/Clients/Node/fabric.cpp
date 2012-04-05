/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Clients/Node/ClientFactory.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Core/Build.h>

#include <llvm/Support/Threading.h>
#include <time.h>
#include <stdlib.h>

namespace Fabric
{
  namespace CLI
  {
    extern "C" FABRIC_CLI_EXPORT void init( v8::Handle<v8::Object> target )
    {
      FABRIC_LOG( "%s version %s", Fabric::buildName, Fabric::buildFullVersion );

      llvm::llvm_start_multithreaded();
      if ( !llvm::llvm_is_multithreaded() )
      {
        FABRIC_LOG( "LLVM not compiled with multithreading enabled; aborting" );
        abort();
      }

      v8::HandleScope handleScope;
      target->Set( v8::String::New("createClient"), CreateClientV8Function() );
    }
  }
}
