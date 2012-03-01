/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Clients/CLI/ClientFactory.h>
#include <Fabric/Clients/CLI/Client.h>
#include <Fabric/Clients/CLI/LogCollector.h>
#include <Fabric/Core/Build.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/Debug.h>
#include <Fabric/Core/RT/NumericDesc.h>
#include <Fabric/Core/RT/StructDesc.h>
#include <Fabric/Core/OCL/OCL.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/EDK/Common.h>

#include <v8.h>
#include <string>

namespace Fabric
{
  namespace CLI
  {
//#define FABRIC_CLI_LOG

#if defined(FABRIC_CLI_LOG)    
    static v8::Handle<v8::Value> LogCallbackV8FunctionCallback( v8::Arguments const &args )
    {
      //FABRIC_LOG( "LogCallbackV8FunctionCallback()" );
      
      v8::HandleScope handleScope;
      
      v8::Handle<v8::String> v8ArgAsString = args[0]->ToString();
      v8::String::Utf8Value v8ArgAsStringUtf8Value( v8ArgAsString );

      FABRIC_LOG( *v8ArgAsStringUtf8Value );
      
      return handleScope.Close( v8::Handle<v8::Value>() );
    }
      
    static v8::Handle<v8::Value> LogCallbackV8Function()
    {
      //FABRIC_LOG( "LogCallbackV8Function()" );
      
      v8::HandleScope handleScope;
      
      v8::Persistent<v8::FunctionTemplate> logCallbackV8FunctionTemplate;
      if ( logCallbackV8FunctionTemplate.IsEmpty() )
      {
        logCallbackV8FunctionTemplate = v8::Persistent<v8::FunctionTemplate>::New( v8::FunctionTemplate::New( &LogCallbackV8FunctionCallback ) );
      }
      
      return handleScope.Close( logCallbackV8FunctionTemplate->GetFunction() );
    }
#endif

    static v8::Handle<v8::Value> CreateClientV8FunctionCallback( v8::Arguments const &args )
    {
      v8::HandleScope handleScope;
      
      v8::Handle<v8::Object> clientWrapConstructorHolder = v8::Object::New();
      ClientWrap::Initialize( clientWrapConstructorHolder );
      
      v8::Handle<v8::Object> clientWrap = v8::Handle<v8::Function>::Cast( clientWrapConstructorHolder->Get( v8::String::New( "Client" ) ) )->NewInstance();
      
      v8::TryCatch v8TryCatch;
      std::string const &wrapFabricClientJSSource = DG::Context::GetWrapFabricClientJSSource();
      v8::Handle<v8::String> wrapFabricClientJSSourceV8String = v8::String::New( wrapFabricClientJSSource.data(), wrapFabricClientJSSource.length() );
      v8::Handle<v8::Script> wrapFabricClientJSSourceV8Script = v8::Script::Compile( wrapFabricClientJSSourceV8String, v8::String::New("FABRIC.Wrappers.js") );
      FABRIC_ASSERT( !wrapFabricClientJSSourceV8Script.IsEmpty() );
      v8::Handle<v8::Value> wrapFabricClientV8Value = wrapFabricClientJSSourceV8Script->Run();
      FABRIC_ASSERT( !wrapFabricClientV8Value.IsEmpty() );
      FABRIC_ASSERT( wrapFabricClientV8Value->IsFunction() );
      v8::Handle<v8::Function> wrapFabricClientV8Function = v8::Handle<v8::Function>::Cast( wrapFabricClientV8Value );
      std::vector< v8::Handle<v8::Value> > argv;
      argv.push_back( clientWrap );
#if defined(FABRIC_CLI_LOG)    
      argv.push_back( v8::Boolean::New( false ) );
      argv.push_back( LogCallbackV8Function() );
#endif
      v8::Handle<v8::Value> result = wrapFabricClientV8Function->Call( args.This(), argv.size(), &argv[0] );
      return handleScope.Close( result );
    }
    
    v8::Handle<v8::Value> CreateClientV8Function()
    {
      //FABRIC_LOG( "CreateClientV8Function()" );
      
      v8::HandleScope handleScope;
      
      v8::Persistent<v8::FunctionTemplate> createClientV8FunctionTemplate;
      if ( createClientV8FunctionTemplate.IsEmpty() )
      {
        createClientV8FunctionTemplate = v8::Persistent<v8::FunctionTemplate>::New( v8::FunctionTemplate::New( &CreateClientV8FunctionCallback ) );
      }
      
      return handleScope.Close( createClientV8FunctionTemplate->GetFunction() );
    }
  };
};
