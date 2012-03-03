/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>
#include <Fabric/Clients/CLI/Client.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Manager.h>
#include <Fabric/Core/IO/ResourceManager.h>
#include <Fabric/Core/IO/SimpleFileHandleManager.h>
#include <Fabric/Core/IO/FileResourceProvider.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/DG/Context.h>
#include <fstream>

#include <node.h>
#include <string>

namespace Fabric
{
  namespace CLI
  {
    class TestSynchronousFileResourceProvider : public IO::ResourceProvider
    {
    public:

      static RC::Handle<TestSynchronousFileResourceProvider> Create()
      {
        return new TestSynchronousFileResourceProvider();
      }

      virtual char const * getUrlScheme() const
      {
        return "testfile";
      }

      virtual void get( char const *url, bool getAsFile, void* userData )
      {
        if( strncmp( "testfile://", url, 11 ) != 0 )
            throw Exception( "Error: URL not properly formatted for local files" );//Don't put filename as it might be private

        std::string fileWithPath = IO::ChangeSeparatorsURLToFile( std::string( url+11 ) );
        if( !IO::FileExists( fileWithPath ) )
            throw Exception( "Error: file doesn't exist" );//Don't put filename as it might be private

        std::string extension = IO::GetExtension( fileWithPath );
        size_t fileSize = IO::GetFileSize( fileWithPath );

        if( getAsFile )
        {
          IO::ResourceManager::onFileAsyncThreadCall( fileWithPath.c_str(), userData );
          IO::ResourceManager::onProgressAsyncThreadCall( extension.c_str(), fileSize, fileSize, userData );
        }
        else
        {
          std::ifstream file( fileWithPath.c_str(), std::ios::in | std::ios::binary );
          if( !file.is_open() )
            IO::ResourceManager::onFailureAsyncThreadCall( "Unable to open file", userData );

          file.exceptions( std::ifstream::badbit );
          try
          {
            size_t fileSize = IO::GetFileSize( fileWithPath.c_str() );
            static const size_t maxReadSize = 1<<16;//64K buffers
            char data[maxReadSize];
            size_t offset = 0;
            while( offset != fileSize )
            {
              size_t nbRead = std::min( maxReadSize, fileSize-offset );
              file.read( data, nbRead );
              IO::ResourceManager::onDataAsyncThreadCall( offset, nbRead, data, userData );
              offset += nbRead;
              IO::ResourceManager::onProgressAsyncThreadCall( "text/plain", offset, fileSize, userData );
            }
          }
          catch(...)
          {
            IO::ResourceManager::onFailureAsyncThreadCall( "Error while reading file", userData );
          }
        }
      }

    private:
      TestSynchronousFileResourceProvider(){}
    };

    class IOManager : public IO::Manager
    {
    public:
    
      static RC::Handle<IOManager> Create( IO::ScheduleAsyncCallbackFunc scheduleFunc, void *scheduleFuncUserData )
      {
        return new IOManager( scheduleFunc, scheduleFuncUserData );
      }

      virtual std::string queryUserFilePath(
        bool existingFile,
        std::string const &title,
        std::string const &defaultFilename,
        std::string const &extension
        ) const
      {
        //For unit test purposes only
        if ( !IO::DirExists( "TMP" ) )
	        IO::CreateDir( "TMP" );
        if(existingFile)
          return IO::JoinPath("TMP", "default.txt");
        else
          return IO::JoinPath("TMP", defaultFilename);
      }
  
    protected:
    
      IOManager( IO::ScheduleAsyncCallbackFunc scheduleFunc, void *scheduleFuncUserData ) : IO::Manager( IO::SimpleFileHandleManager::Create(), scheduleFunc, scheduleFuncUserData )
      {
        getResourceManager()->registerProvider( RC::Handle<IO::ResourceProvider>::StaticCast( TestSynchronousFileResourceProvider::Create() ), false );
        getResourceManager()->registerProvider( RC::Handle<IO::ResourceProvider>::StaticCast( IO::FileResourceProvider::Create( true ) ), true );
      }
    };
    
    RC::Handle<Client> Client::Create( RC::Handle<DG::Context> const &context, ClientWrap *clientWrap )
    {
      return new Client( context, clientWrap );
    }
    
    Client::Client( RC::Handle<DG::Context> const &context, ClientWrap *clientWrap )
      : DG::Client( context )
      , m_clientWrap( clientWrap )
    {
    }
    
    void Client::invalidate()
    {
      m_clientWrap = 0;
    }
    
    Client::~Client()
    {
      invalidate();
    }
    
    void Client::notify( Util::SimpleString const &jsonEncodedNotifications ) const
    {
      if ( m_clientWrap )
        m_clientWrap->notify( jsonEncodedNotifications.data(), jsonEncodedNotifications.length() );
    }
    
    void ClientWrap::Initialize( v8::Handle<v8::Object> target )
    {
      v8::HandleScope v8HandleScope;
      
      node::HandleWrap::Initialize( target );
    
      v8::Local<v8::FunctionTemplate> v8FunctionTemplate = v8::FunctionTemplate::New( New );
      v8FunctionTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );
      v8FunctionTemplate->SetClassName( v8::String::New( "Client" ) );
      
      NODE_SET_PROTOTYPE_METHOD( v8FunctionTemplate, "jsonExec", JSONExec );
      NODE_SET_PROTOTYPE_METHOD( v8FunctionTemplate, "setJSONNotifyCallback", SetJSONNotifyCallback );
      NODE_SET_PROTOTYPE_METHOD( v8FunctionTemplate, "close", Close );
      
      target->Set( v8::String::New( "Client" ), v8FunctionTemplate->GetFunction() );
    }
    
    v8::Handle<v8::Value> ClientWrap::New( v8::Arguments const &args )
    {
      // This constructor should not be exposed to public javascript.
      // Therefore we assert that we are not trying to call this as a
      // normal function.
      FABRIC_ASSERT( args.IsConstructCall() );

      v8::HandleScope v8HandleScope;
      ClientWrap *clientWrap = new ClientWrap( args.This() );
      FABRIC_ASSERT( clientWrap );
      
      return v8HandleScope.Close( args.This() );
    }
    
    ClientWrap::ClientWrap( v8::Handle<v8::Object> object )
      : node::HandleWrap( object, 0 )
      , m_mutex("Node.js ClientWrap")
    {
      std::vector<std::string> pluginPaths;
#if defined(FABRIC_OS_MACOSX)
      char const *home = getenv("HOME");
      if ( home && *home )
      {
        std::string homePath( home );
        pluginPaths.push_back( IO::JoinPath( homePath, "Library", "Fabric", "Exts" ) );
      }
      pluginPaths.push_back( "/Library/Fabric/Exts" );
#elif defined(FABRIC_OS_LINUX)
      char const *home = getenv("HOME");
      if ( home && *home )
      {
        std::string homePath( home );
        pluginPaths.push_back( IO::JoinPath( homePath, ".fabric", "Exts" ) );
      }
      pluginPaths.push_back( "/usr/lib/fabric/Exts" );
#elif defined(FABRIC_OS_WINDOWS)
      char const *appData = getenv("APPDATA");
      if ( appData && *appData )
      {
        std::string appDataDir(appData);
        pluginPaths.push_back( IO::JoinPath( appDataDir, "Fabric" , "Exts" ) );
      }
#endif

      CG::CompileOptions compileOptions;
      compileOptions.setGuarded( false );

      RC::Handle<IO::Manager> ioManager = IOManager::Create( &ClientWrap::ScheduleAsyncUserCallback, this );
      RC::Handle<DG::Context> dgContext = DG::Context::Create( ioManager, pluginPaths, compileOptions, true );
#if defined(FABRIC_MODULE_OPENCL)
      OCL::registerTypes( dgContext->getRTManager() );
#endif

      Plug::Manager::Instance()->loadBuiltInPlugins( pluginPaths, dgContext->getCGManager(), DG::Context::GetCallbackStruct() );
      
      m_client = Client::Create( dgContext, this );

      m_mainThreadTLS = true;
      uv_async_init( uv_default_loop(), &m_uvAsync, &ClientWrap::AsyncCallback );
      SetHandle( (uv_handle_t *)&m_uvAsync );
      
      //// uv_timer_init adds a loop reference. (That is, it calls uv_ref.) This
      //// is not the behavior we want in Node. Timers should not increase the
      //// ref count of the loop except when active.
      //uv_unref( uv_default_loop() );
    }

    ClientWrap::~ClientWrap()
    {
      //if (!active_)
      //  uv_ref( uv_default_loop() );

      m_notifyCallback.Dispose();
    }
    
    void ClientWrap::AsyncCallback( uv_async_t *async, int status )
    {
      ClientWrap *clientWrap = static_cast<ClientWrap *>( async->data );
      Util::Mutex::Lock lock( clientWrap->m_mutex );
      for ( std::vector<std::string>::const_iterator it=clientWrap->m_bufferedNotifications.begin(); it!=clientWrap->m_bufferedNotifications.end(); ++it )
        clientWrap->notify( it->data(), it->length() );
      clientWrap->m_bufferedNotifications.clear();
      for ( std::vector<AsyncCallbackData>::const_iterator it2=clientWrap->m_bufferedAsyncUserCallbacks.begin(); it2!=clientWrap->m_bufferedAsyncUserCallbacks.end(); ++it2 )
        (*(it2->m_callbackFunc))( it2->m_callbackFuncUserData );
      clientWrap->m_bufferedAsyncUserCallbacks.clear();
    }
    
#define UNWRAP \
  FABRIC_ASSERT( !args.Holder().IsEmpty() ); \
  FABRIC_ASSERT( args.Holder()->InternalFieldCount() > 0 ); \
  ClientWrap* wrap = static_cast<ClientWrap *>( args.Holder()->GetPointerFromInternalField(0) ); \
  if ( !wrap ) \
    return v8::ThrowException( v8::String::New( "client has already been closed" ) );

    v8::Handle<v8::Value> ClientWrap::JSONExec( v8::Arguments const &args )
    {
      UNWRAP
      
      if ( args.Length() != 1 || !args[0]->IsString() )
        return v8::ThrowException( v8::String::New( "jsonExec: takes one string parameter (jsonEncodedCommands)" ) );
      
      v8::HandleScope v8HandleScope;
      v8::Handle<v8::String> v8JSONEncodedCommands = v8::Handle<v8::String>::Cast( args[0] );
      v8::String::Utf8Value v8JSONEncodedCommandsUtf8Value( v8JSONEncodedCommands );
      Util::SimpleString jsonEncodedResults;
      {
        v8::Handle<v8::Object> v8This = args.This();
        JSON::Encoder resultJSON( &jsonEncodedResults );
        wrap->m_client->jsonExec(
          const_cast<char const *>(*v8JSONEncodedCommandsUtf8Value),
          size_t(v8JSONEncodedCommandsUtf8Value.length()),
          resultJSON
          );
      }
      v8::Handle<v8::String> v8JSONEncodedResults = v8::String::New( jsonEncodedResults.data(), jsonEncodedResults.length() );
      return v8HandleScope.Close( v8JSONEncodedResults );
    }
      
    v8::Handle<v8::Value> ClientWrap::SetJSONNotifyCallback( v8::Arguments const &args )
    {
      UNWRAP
      
      if ( args.Length() != 1 || !args[0]->IsFunction() )
        return v8::ThrowException( v8::String::New( "setJSONNotifyCallback: takes 1 function parameter (notificationCallback)" ) );
      v8::Handle<v8::Function> v8Callback = v8::Handle<v8::Function>::Cast( args[0] );

      v8::HandleScope v8HandleScope;
      wrap->m_notifyCallback = v8::Persistent<v8::Function>::New( v8Callback );
      wrap->m_client->notifyInitialState();
      return v8HandleScope.Close( v8::Handle<v8::Value>() );
    }
    
    v8::Handle<v8::Value> ClientWrap::Close( v8::Arguments const &args )
    {
      UNWRAP
      node::HandleWrap::Close( args );
      if ( wrap->m_client )
      {
        wrap->m_client->invalidate();
        wrap->m_client = 0;
      }
      return v8::Handle<v8::Value>();
    }
    
    void ClientWrap::notify( char const *data, size_t length )
    {
      Util::Mutex::Lock lock(m_mutex);
      if ( m_mainThreadTLS )
      {
        v8::HandleScope v8HandleScope;
        v8::Handle<v8::Value> jsonEncodedNotificationsV8String = v8::String::New( data, length );
        m_notifyCallback->Call( m_notifyCallback, 1, &jsonEncodedNotificationsV8String );
      }
      else
      {
        m_bufferedNotifications.push_back( std::string( data, length ) );
        uv_async_send( &m_uvAsync );
      }
    }
    
    void ClientWrap::ScheduleAsyncUserCallback( void* scheduleUserData, void (*callbackFunc)(void *), void *callbackFuncUserData )
    {
      ClientWrap *clientWrap = static_cast<ClientWrap *>( scheduleUserData );
      if ( clientWrap->m_mainThreadTLS )//[JeromeCG 20111221] I didn't want to call synchronously even if in main thread, however calling uv_async_send seems to cause a crash??
        (*callbackFunc)(callbackFuncUserData);
      else {
        Util::Mutex::Lock lock( clientWrap->m_mutex );
        AsyncCallbackData cbData;
        cbData.m_callbackFunc = callbackFunc;
        cbData.m_callbackFuncUserData = callbackFuncUserData;
        clientWrap->m_bufferedAsyncUserCallbacks.push_back( cbData );//Note: even if m_mainThreadTLS
        uv_async_send( &clientWrap->m_uvAsync );
      }
    }    
  };
};
