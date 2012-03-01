/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/MT/Impl.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>

#if !defined(FABRIC_OS_WINDOWS)
# include <alloca.h>
#endif

#include <string.h>

namespace Fabric
{
  namespace MR
  {
    ValueProducer::ValueProducer()
    {
    }
    
    ValueProducer::ComputeState::ComputeState( RC::ConstHandle<ValueProducer> const &valueProducer )
      : m_valueProducer( valueProducer )
    {
    }
    
    void ValueProducer::ComputeState::produceJSON( JSON::Encoder &jg ) const
    {
      RC::ConstHandle<RT::Desc> valueDesc = m_valueProducer->getValueDesc();
      
      size_t allocSize = valueDesc->getAllocSize();
      void *valueData = alloca( allocSize );
      memset( valueData, 0, allocSize );
      produce( valueData );
      valueDesc->encodeJSON( valueData, jg );
      valueDesc->disposeData( valueData );
    }
    
    struct ProduceJSONAsyncCallbackData
    {
      RC::Handle<ValueProducer::ComputeState> computeState;
      JSON::ObjectEncoder *jsonObjectEncoder;
    };

    void ValueProducer::ComputeState::produceJSONAsync(
      JSON::ObjectEncoder &jsonObjectEncoder,
      void (*finishedCallback)( void * ),
      void *finishedUserdata
      )
    {
      ProduceJSONAsyncCallbackData *produceJSONAsyncCallbackData = new ProduceJSONAsyncCallbackData;
      produceJSONAsyncCallbackData->computeState = this;
      produceJSONAsyncCallbackData->jsonObjectEncoder = &jsonObjectEncoder;
      
      MT::ThreadPool::Instance()->executeParallelAsync(
        MT::tlsLogCollector.get(),
        1,
        &ValueProducer::ComputeState::ProduceJSONAsyncCallback,
        produceJSONAsyncCallbackData,
        MT::ThreadPool::Idle,
        finishedCallback,
        finishedUserdata
        );
    }
    
    void ValueProducer::ComputeState::ProduceJSONAsyncCallback(
      void *userdata,
      size_t index
      )
    {
      ProduceJSONAsyncCallbackData *produceJSONAsyncCallbackData = static_cast<ProduceJSONAsyncCallbackData *>( userdata );
      {
        JSON::Encoder jg = produceJSONAsyncCallbackData->jsonObjectEncoder->makeMember( "result" );
        produceJSONAsyncCallbackData->computeState->produceJSON( jg );
      }
      delete produceJSONAsyncCallbackData;
    }
  };
};
