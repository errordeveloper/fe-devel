/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>
#include <Fabric/Clients/Python/Client.h>
#include <Fabric/Clients/Python/ClientWrap.h>
#include <Fabric/Clients/Python/IOManager.h>
#include <Fabric/Core/Plug/Helpers.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Manager.h>
#include <Fabric/Core/IO/ResourceManager.h>
#include <Fabric/Core/IO/FileHandleManager.h>
#include <Fabric/Core/IO/FileHandleResourceProvider.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/MT/Util.h>

#include <vector>
#include <string>

namespace Fabric
{
  namespace Python
  {
    ClientWrap::ClientWrap( char const *opts )
      : m_mutex( "Python ClientWrap" )
    {
      std::vector<std::string> pluginPaths;
      Plug::AppendUserPaths( pluginPaths );
      Plug::AppendGlobalPaths( pluginPaths );

      CG::CompileOptions compileOptions;
      compileOptions.setGuarded( false );
      int logWarnings = -1;

      if ( opts )
      {
        JSON::Decoder decoder( opts, strlen( opts ) );
        JSON::Entity entity;
        if ( decoder.getNext( entity ) )
        {
          JSON::ObjectDecoder argObjectDecoder( entity );
          JSON::Entity keyString, valueEntity;
          while ( argObjectDecoder.getNext( keyString, valueEntity ) )
          {
            try
            {
              if ( keyString.stringIs( "logWarnings", 11 ) )
              {
                valueEntity.requireBoolean();
                logWarnings = valueEntity.booleanValue();
              }
              else if ( keyString.stringIs( "guarded", 7 ) )
              {
                valueEntity.requireBoolean();
                compileOptions.setGuarded( valueEntity.booleanValue() );
              }
            }
            catch ( Exception e ) {}
          }
        }
      }

      RC::Handle<IO::Manager> ioManager = IOManager::Create( &ClientWrap::ScheduleAsyncUserCallback, this );
      RC::Handle<DG::Context> dgContext = DG::Context::Create( ioManager, pluginPaths, compileOptions, true );
#if defined(FABRIC_MODULE_OPENCL)
      OCL::registerTypes( dgContext->getRTManager() );
#endif
      
      if ( logWarnings > -1 )
        dgContext->setLogWarnings( logWarnings );

      Plug::Manager::Instance()->loadBuiltInPlugins( pluginPaths, dgContext->getCGManager(), DG::Context::GetCallbackStruct() );

      m_client = Client::Create( dgContext, this );

      m_mainThreadTLS = true;
    }

    ClientWrap::~ClientWrap()
    {
      PassedStringMap::iterator it = m_passedStrings.begin();
      while ( it != m_passedStrings.end() )
      {
        m_passedStrings.erase( it );
        delete it->second;
        it++;
      }
      FABRIC_ASSERT( m_passedStrings.empty() );
    }

    void ClientWrap::setJSONNotifyCallback( void (*callback)(const char *) )
    {
      FABRIC_ASSERT( m_client );

      m_notifyCallback = callback;
      m_client->notifyInitialState();
    }

    void ClientWrap::notify( Util::SimpleString const &jsonEncodedNotifications ) const
    {
      m_notifyCallback( jsonEncodedNotifications.c_str() );
    }

    void ClientWrap::jsonExecAndAllocCStr( char const *data, size_t length, const char **str )
    {
      FABRIC_ASSERT( m_client );

      Util::SimpleString *jsonEncodedResults = new Util::SimpleString();
      JSON::Encoder resultJSON( jsonEncodedResults );

      m_client->jsonExec( data, length, resultJSON );

      *str = jsonEncodedResults->c_str();

      // keep a handle to the SimpleString for later deletion
      PassedStringMap::iterator i = m_passedStrings.find( *str );
      FABRIC_ASSERT( i == m_passedStrings.end() );
      m_passedStrings[ *str ] = jsonEncodedResults;
    }

    void ClientWrap::freeJsonCStr( char const *str )
    {
      PassedStringMap::iterator i = m_passedStrings.find( str );
      FABRIC_ASSERT( i != m_passedStrings.end() );
      delete i->second;
      m_passedStrings.erase( i );
    }

    void ClientWrap::ScheduleAsyncUserCallback(
        void *scheduleUserData,
        void (*callbackFunc)(void *),
        void *callbackFuncUserData
        )
    {
      ClientWrap *clientWrap = static_cast<ClientWrap *>( scheduleUserData );
      if ( clientWrap->m_mainThreadTLS )
      {
        // andrew 2012-02-02
        // this is the same as Node.js for now, this may need to be
        // re-evaluated later (running directly on main thread)
        callbackFunc(callbackFuncUserData);
      }
      else
      {
        Util::Mutex::Lock lock( clientWrap->m_mutex );
        AsyncCallbackData cbData;
        cbData.m_callbackFunc = callbackFunc;
        cbData.m_callbackFuncUserData = callbackFuncUserData;
        clientWrap->m_bufferedAsyncUserCallbacks.push_back( cbData );
        clientWrap->runScheduledCallbacksNotify();
      }
    }

    void ClientWrap::runScheduledCallbacksNotify()
    {
      // FIXME do something more sensible
      notify( Util::SimpleString("[{\"src\":[\"ClientWrap\"],\"cmd\":\"runScheduledCallbacks\"}]") );
    }

    void ClientWrap::runScheduledCallbacks()
    {
      Util::Mutex::Lock lock( m_mutex );
      for ( std::vector<AsyncCallbackData>::const_iterator it=m_bufferedAsyncUserCallbacks.begin(); it!=m_bufferedAsyncUserCallbacks.end(); ++it )
      {
        (*(it->m_callbackFunc))( it->m_callbackFuncUserData );
      }
      m_bufferedAsyncUserCallbacks.clear();
    }
  }
};
