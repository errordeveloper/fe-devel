/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayTransformWrapper.h>
#include <Fabric/Core/MR/ArrayTransform.h>
#include <Fabric/Core/MR/ArrayTransformOperator.h>
#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Core/MR/ArrayProducerWrapper.h>
#include <Fabric/Core/KLC/ArrayTransformOperator.h>
#include <Fabric/Core/KLC/ArrayTransformOperatorWrapper.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ArrayTransformWrapper, ArrayProducerWrapper );
    
    RC::Handle<ArrayTransformWrapper> ArrayTransformWrapper::Create(
      RC::ConstHandle<ArrayProducerWrapper> const &inputArrayProducer,
      RC::ConstHandle<KLC::ArrayTransformOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
    {
      return new ArrayTransformWrapper( FABRIC_GC_OBJECT_MY_CLASS, inputArrayProducer, operator_, sharedValueProducer );
    }
    
    ArrayTransformWrapper::ArrayTransformWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<ArrayProducerWrapper> const &inputArrayProducer,
      RC::ConstHandle<KLC::ArrayTransformOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
      : ArrayProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_inputArrayProducer( inputArrayProducer )
      , m_operator( operator_ )
      , m_sharedValueProducer( sharedValueProducer )
      , m_unwrapped(
        ArrayTransform::Create(
          inputArrayProducer->getUnwrapped(),
          operator_->getUnwrapped(),
          sharedValueProducer? sharedValueProducer->getUnwrapped(): RC::ConstHandle<ValueProducer>()
          )
        )
    {
    }
      
    RC::ConstHandle<ArrayProducer> ArrayTransformWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ArrayTransformWrapper::getKind() const
    {
      return "ArrayTransform";
    }
    
    void ArrayTransformWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
    {
      {
        JSON::Encoder jg = objectEncoder.makeMember( "input" );
        m_inputArrayProducer->toJSON( jg );
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
