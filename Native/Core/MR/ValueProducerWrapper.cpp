/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ValueProducerWrapper, ProducerWrapper );
    
    ValueProducerWrapper::ValueProducerWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM
      )
      : ProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
    {
    }
    
    void ValueProducerWrapper::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "produce", 7 ) )
        jsonExecProduce( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "produceAsync", 12 ) )
        jsonExecProduceAsync( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "flush", 5 ) )
        jsonExecFlush( arg, resultArrayEncoder );
      else ProducerWrapper::jsonExec( cmd, arg, resultArrayEncoder );
    }
    
    void ValueProducerWrapper::jsonExecProduce(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      JSON::Encoder jg = resultArrayEncoder.makeElement();
      getUnwrapped()->createComputeState()->produceJSON( jg );
    }
    
    struct JSONProduceAsyncUserdata
    {
      RC::Handle<ValueProducerWrapper> valueProducerWrapper;
      Util::SimpleString *notifyJSONArg;
      JSON::Encoder notifyJSONArgEncoder;
      JSON::ObjectEncoder notifyJSONArgObjectEncoder;
    
      JSONProduceAsyncUserdata( RC::Handle<ValueProducerWrapper> const &valueProducerWrapper_, int32_t serial )
        : valueProducerWrapper( valueProducerWrapper_ )
        , notifyJSONArg( new Util::SimpleString )
        , notifyJSONArgEncoder( notifyJSONArg )
        , notifyJSONArgObjectEncoder( notifyJSONArgEncoder.makeObject() )
      {
        notifyJSONArgObjectEncoder.makeMember( "serial" ).makeInteger( serial );
      }
    };
    
    void ValueProducerWrapper::jsonExecProduceAsync( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireInteger();
      JSONProduceAsyncUserdata *jsonProduceAsyncUserdata = new JSONProduceAsyncUserdata( this, arg.integerValue() );
      getUnwrapped()->createComputeState()->produceJSONAsync(
        jsonProduceAsyncUserdata->notifyJSONArgObjectEncoder,
        &ValueProducerWrapper::JSONExecProduceAsyncFinishedCallback,
        jsonProduceAsyncUserdata
        );
    }
    
    void ValueProducerWrapper::JSONExecProduceAsyncFinishedCallback( void *userdata )
    {
      JSONProduceAsyncUserdata *jsonProduceAsyncUserdata = static_cast<JSONProduceAsyncUserdata *>( userdata );
      RC::Handle<ValueProducerWrapper> valueProducerWrapper = jsonProduceAsyncUserdata->valueProducerWrapper;
      Util::SimpleString *notifyJSONArg = jsonProduceAsyncUserdata->notifyJSONArg;
      delete jsonProduceAsyncUserdata;

      valueProducerWrapper->jsonNotify( "produceAsyncFinished", 20, notifyJSONArg );
      delete notifyJSONArg;
    }

    void ValueProducerWrapper::jsonExecFlush(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      (const_cast<ValueProducer *>(getUnwrapped().ptr()))->flush();
    }
  }
}
