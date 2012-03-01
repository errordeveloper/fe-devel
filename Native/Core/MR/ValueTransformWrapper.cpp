/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueTransformWrapper.h>
#include <Fabric/Core/MR/ValueTransform.h>
#include <Fabric/Core/MR/ValueTransformOperator.h>
#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Core/KLC/ValueTransformOperator.h>
#include <Fabric/Core/KLC/ValueTransformOperatorWrapper.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ValueTransformWrapper, ValueProducerWrapper );
    
    RC::Handle<ValueTransformWrapper> ValueTransformWrapper::Create(
      RC::ConstHandle<ValueProducerWrapper> const &inputValueProducer,
      RC::ConstHandle<KLC::ValueTransformOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
    {
      return new ValueTransformWrapper( FABRIC_GC_OBJECT_MY_CLASS, inputValueProducer, operator_, sharedValueProducer );
    }
    
    ValueTransformWrapper::ValueTransformWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<ValueProducerWrapper> const &inputValueProducer,
      RC::ConstHandle<KLC::ValueTransformOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
      : ValueProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_inputValueProducer( inputValueProducer )
      , m_operator( operator_ )
      , m_sharedValueProducer( sharedValueProducer )
      , m_unwrapped(
        ValueTransform::Create(
          inputValueProducer->getUnwrapped(),
          operator_->getUnwrapped(),
          sharedValueProducer? sharedValueProducer->getUnwrapped(): RC::ConstHandle<ValueProducer>()
          )
        )
    {
    }
      
    RC::ConstHandle<ValueProducer> ValueTransformWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ValueTransformWrapper::getKind() const
    {
      return "ValueTransform";
    }
    
    void ValueTransformWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
    {
      {
        JSON::Encoder jg = objectEncoder.makeMember( "input" );
        m_inputValueProducer->toJSON( jg );
      }

      {
        JSON::Encoder jg = objectEncoder.makeMember( "operator" );
        m_operator->toJSON( jg );
      }

      if ( m_sharedValueProducer )
      {
        JSON::Encoder jg = objectEncoder.makeMember( "shared" );
        m_sharedValueProducer->toJSON( jg );
      }
    }
  }
}
