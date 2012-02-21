/*
 *  Copyright 2010-2012 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Base/Util/SimpleString.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Clients/Python/ClientWrap.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Core/Build.h>

#include <map>
#include <time.h>

//#define FABRIC_PYTHON_DEBUG

namespace Fabric
{
  namespace Python
  {
    extern "C" FABRIC_CLI_EXPORT void identify()
    {
      FABRIC_LOG( "%s version %s", Fabric::buildName, Fabric::buildFullVersion );
      struct tm const *lt = localtime( &Fabric::buildExpiry );
      char buf[1024];
      strftime( buf, 1024, "This build of Fabric will expire on %Y-%m-%d at %H:%M:%S", lt );
      FABRIC_LOG( "%s", buf );
    }

    extern "C" FABRIC_CLI_EXPORT void createClient( void **ptr )
    {
      ClientWrap *client = new ClientWrap();

#ifdef FABRIC_PYTHON_DEBUG
      FABRIC_LOG( "calling createClient: %lx", client);
#endif

      client->retain();
      *ptr = client;
    }
    
    extern "C" FABRIC_CLI_EXPORT void jsonExec( void *client_, char const *data, size_t length, const char **result )
    {
#ifdef FABRIC_PYTHON_DEBUG
      FABRIC_LOG( "calling jsonExec: %lx", client_ );
#endif

      ClientWrap *client( static_cast<ClientWrap *>( client_ ) );

#ifdef FABRIC_PYTHON_DEBUG
      FABRIC_LOG( "sending to jsonExec: %s", data );
#endif

      client->jsonExecAndAllocCStr( data, length, result );

#ifdef FABRIC_PYTHON_DEBUG
      FABRIC_LOG( "received from jsonExec: %s", *result );
#endif
    }

    extern "C" FABRIC_CLI_EXPORT void freeString( void *client_, void *string )
    {
#ifdef FABRIC_PYTHON_DEBUG
      FABRIC_LOG( "calling freeString: %lx (%lx)", client_, string );
#endif

      ClientWrap *client( static_cast<ClientWrap *>( client_ ) );
      client->freeJsonCStr( (const char *)string );
    }

    extern "C" FABRIC_CLI_EXPORT void setJSONNotifyCallback(
        void *client_,
        void (*callback)(const char *)
        )
    {
#ifdef FABRIC_PYTHON_DEBUG
      FABRIC_LOG( "calling setJSONNotifyCallback: %lx", client_ );
#endif

      ClientWrap *client( static_cast<ClientWrap *>( client_ ) );
      client->setJSONNotifyCallback( callback );
    }

    extern "C" FABRIC_CLI_EXPORT void freeClient( void *client_ )
    {
#ifdef FABRIC_PYTHON_DEBUG
      FABRIC_LOG( "calling close: %lx", client_ );
#endif

      ClientWrap *client( static_cast<ClientWrap *>( client_ ) );
      client->release();
    }

    extern "C" FABRIC_CLI_EXPORT void runScheduledCallbacks( void *client_ )
    {
#ifdef FABRIC_PYTHON_DEBUG
      FABRIC_LOG( "calling runScheduledCallbacks: %lx", client_ );
#endif

      ClientWrap *client( static_cast<ClientWrap *>( client_ ) );
      client->runScheduledCallbacks();
    }
  };
};
