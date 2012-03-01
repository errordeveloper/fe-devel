/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayMapWrapper.h>
#include <Fabric/Core/MR/ArrayMap.h>
#include <Fabric/Core/MR/ArrayMapOperator.h>
#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Core/MR/ArrayProducerWrapper.h>
#include <Fabric/Core/KLC/ArrayMapOperator.h>
#include <Fabric/Core/KLC/ArrayMapOperatorWrapper.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ArrayMapWrapper, ArrayProducerWrapper );
    
    RC::Handle<ArrayMapWrapper> ArrayMapWrapper::Create(
      RC::ConstHandle<ArrayProducerWrapper> const &inputArrayProducer,
      RC::ConstHandle<KLC::ArrayMapOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
    {
      return new ArrayMapWrapper( FABRIC_GC_OBJECT_MY_CLASS, inputArrayProducer, operator_, sharedValueProducer );
    }
    
    ArrayMapWrapper::ArrayMapWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<ArrayProducerWrapper> const &inputArrayProducer,
      RC::ConstHandle<KLC::ArrayMapOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
      : ArrayProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_inputArrayProducer( inputArrayProducer )
      , m_operator( operator_ )
      , m_sharedValueProducer( sharedValueProducer )
      , m_unwrapped(
        ArrayMap::Create(
          inputArrayProducer->getUnwrapped(),
          operator_->getUnwrapped(),
          sharedValueProducer? sharedValueProducer->getUnwrapped(): RC::ConstHandle<ValueProducer>()
          )
        )
    {
    }
      
    RC::ConstHandle<ArrayProducer> ArrayMapWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ArrayMapWrapper::getKind() const
    {
      return "ArrayMap";
    }
    
    void ArrayMapWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
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
