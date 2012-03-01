/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueMapWrapper.h>
#include <Fabric/Core/MR/ValueMap.h>
#include <Fabric/Core/MR/ValueMapOperator.h>
#include <Fabric/Core/KLC/ValueMapOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueMapOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ValueMapWrapper, ValueProducerWrapper );
    
    RC::Handle<ValueMapWrapper> ValueMapWrapper::Create(
      RC::ConstHandle<ValueProducerWrapper> const &inputWrapper,
      RC::ConstHandle<KLC::ValueMapOperatorWrapper> const &operatorWrapper,
      RC::ConstHandle<ValueProducerWrapper> const &sharedWrapper
      )
    {
      return new ValueMapWrapper( FABRIC_GC_OBJECT_MY_CLASS, inputWrapper, operatorWrapper, sharedWrapper );
    }
    
    ValueMapWrapper::ValueMapWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<ValueProducerWrapper> const &inputWrapper,
      RC::ConstHandle<KLC::ValueMapOperatorWrapper> const &operatorWrapper,
      RC::ConstHandle<ValueProducerWrapper> const &sharedWrapper
      )
      : ValueProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_inputWrapper( inputWrapper )
      , m_operatorWrapper( operatorWrapper )
      , m_sharedWrapper( sharedWrapper )
      , m_unwrapped(
        ValueMap::Create(
          inputWrapper->getUnwrapped(),
          operatorWrapper->getUnwrapped(),
          sharedWrapper? sharedWrapper->getUnwrapped(): RC::ConstHandle<ValueProducer>()
        )
      )
    {
    }
      
    RC::ConstHandle<ValueProducer> ValueMapWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ValueMapWrapper::getKind() const
    {
      return "ValueMap";
    }
    
    void ValueMapWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
    {
      {
        JSON::Encoder jg = objectEncoder.makeMember( "input" );
        m_inputWrapper->toJSON( jg );
      }

      {
        JSON::Encoder jg = objectEncoder.makeMember( "operator" );
        m_operatorWrapper->toJSON( jg );
      }
      
      if ( m_sharedWrapper )
      {
        JSON::Encoder jg = objectEncoder.makeMember( "shared" );
        m_sharedWrapper->toJSON( jg );
      }
    }
  }
}
