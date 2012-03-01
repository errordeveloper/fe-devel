/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayProducerWrapper.h>
#include <Fabric/Core/MR/ArrayProducer.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ArrayProducerWrapper, ProducerWrapper )
    
    ArrayProducerWrapper::ArrayProducerWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM
      )
      : ProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
    {
    }
    
    void ArrayProducerWrapper::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "getCount", 8 ) )
        jsonExecGetCount( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "produce", 7 ) )
        jsonExecProduce( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "produceAsync", 12 ) )
        jsonExecProduceAsync( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "flush", 5 ) )
        jsonExecFlush( arg, resultArrayEncoder );
      else ProducerWrapper::jsonExec( cmd, arg, resultArrayEncoder );
    }
    
    void ArrayProducerWrapper::jsonExecGetCount(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      JSON::Encoder jg = resultArrayEncoder.makeElement();
      jg.makeInteger( getUnwrapped()->createComputeState()->getCount() );
    }
    
    void ArrayProducerWrapper::jsonExecProduce(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      size_t index = SIZE_MAX;
      size_t count = SIZE_MAX;
      
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "index", 5 ) )
          {
            valueEntity.requireInteger();
            int32_t indexInt32 = valueEntity.integerValue();
            if ( indexInt32 < 0 )
              throw Exception( "out of range" );
            index = size_t( indexInt32 );
          }
          else if ( keyString.stringIs( "count", 5 ) )
          {
            valueEntity.requireInteger();
            int32_t countInt32 = valueEntity.integerValue();
            if ( countInt32 < 0 )
              throw Exception( "out of range" );
            count = size_t( countInt32 );
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      if ( index != SIZE_MAX )
      {
        if ( count != SIZE_MAX )
          getUnwrapped()->createComputeState()->produceJSON( index, count, resultEncoder );
        else getUnwrapped()->createComputeState()->produceJSON( index, resultEncoder );
      }
      else getUnwrapped()->createComputeState()->produceJSON( resultEncoder );
    }
    
    struct JSONProduceAsyncUserdata
    {
      RC::Handle<ArrayProducerWrapper> arrayProducerWrapper;
      Util::SimpleString *notifyJSONArg;
      JSON::Encoder notifyJSONArgEncoder;
      JSON::ObjectEncoder notifyJSONArgObjectEncoder;
    
      JSONProduceAsyncUserdata( RC::Handle<ArrayProducerWrapper> const &arrayProducerWrapper_, int32_t serial )
        : arrayProducerWrapper( arrayProducerWrapper_ )
        , notifyJSONArg( new Util::SimpleString )
        , notifyJSONArgEncoder( notifyJSONArg )
        , notifyJSONArgObjectEncoder( notifyJSONArgEncoder.makeObject() )
      {
        notifyJSONArgObjectEncoder.makeMember( "serial" ).makeInteger( serial );
      }
    };
    
    void ArrayProducerWrapper::jsonExecProduceAsync(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      bool haveSerial = false;
      int32_t serial;
      size_t index = SIZE_MAX;
      size_t count = SIZE_MAX;
      
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "serial", 6 ) )
          {
            valueEntity.requireInteger();
            serial = valueEntity.integerValue();
            haveSerial = true;
          }
          else if ( keyString.stringIs( "index", 5 ) )
          {
            valueEntity.requireInteger();
            int32_t indexInt32 = valueEntity.integerValue();
            if ( indexInt32 < 0 )
              throw Exception( "out of range" );
            index = size_t( indexInt32 );
          }
          else if ( keyString.stringIs( "count", 5 ) )
          {
            valueEntity.requireInteger();
            int32_t countInt32 = valueEntity.integerValue();
            if ( countInt32 < 0 )
              throw Exception( "out of range" );
            count = size_t( countInt32 );
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( !haveSerial )
        throw Exception( "missing 'serial'" );
      JSONProduceAsyncUserdata *jsonProduceAsyncUserdata = new JSONProduceAsyncUserdata( this, serial );
      
      if ( index != SIZE_MAX )
      {
        if ( count != SIZE_MAX )
        {
          getUnwrapped()->createComputeState()->produceJSONAsync(
            index,
            count,
            jsonProduceAsyncUserdata->notifyJSONArgObjectEncoder,
            &ArrayProducerWrapper::JSONExecProduceAsyncFinishedCallback,
            jsonProduceAsyncUserdata
            );
        }
        else getUnwrapped()->createComputeState()->produceJSONAsync(
          index,
          jsonProduceAsyncUserdata->notifyJSONArgObjectEncoder,
          &ArrayProducerWrapper::JSONExecProduceAsyncFinishedCallback,
          jsonProduceAsyncUserdata
          );
      }
      else getUnwrapped()->createComputeState()->produceJSONAsync(
        jsonProduceAsyncUserdata->notifyJSONArgObjectEncoder,
        &ArrayProducerWrapper::JSONExecProduceAsyncFinishedCallback,
        jsonProduceAsyncUserdata
        );
    }
    
    void ArrayProducerWrapper::JSONExecProduceAsyncFinishedCallback( void *userdata )
    {
      JSONProduceAsyncUserdata *jsonProduceAsyncUserdata = static_cast<JSONProduceAsyncUserdata *>( userdata );
      RC::Handle<ArrayProducerWrapper> arrayProducerWrapper = jsonProduceAsyncUserdata->arrayProducerWrapper;
      Util::SimpleString *notifyJSONArg = jsonProduceAsyncUserdata->notifyJSONArg;
      delete jsonProduceAsyncUserdata;

      arrayProducerWrapper->jsonNotify( "produceAsyncFinished", 20, notifyJSONArg );
      delete notifyJSONArg;
    }

    void ArrayProducerWrapper::jsonExecFlush(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      (const_cast<ArrayProducer *>(getUnwrapped().ptr()))->flush();
    }
  }
}
