/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ReduceWrapper.h>
#include <Fabric/Core/MR/Reduce.h>
#include <Fabric/Core/MR/ReduceOperator.h>
#include <Fabric/Core/MR/ArrayProducer.h>
#include <Fabric/Core/MR/ArrayProducerWrapper.h>
#include <Fabric/Core/KLC/ReduceOperatorWrapper.h>
#include <Fabric/Core/KLC/ReduceOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ReduceWrapper, ValueProducerWrapper );
    
    RC::Handle<ReduceWrapper> ReduceWrapper::Create(
      RC::ConstHandle<ArrayProducerWrapper> const &inputWrapper,
      RC::ConstHandle<KLC::ReduceOperatorWrapper> const &operatorWrapper,
      RC::ConstHandle<ValueProducerWrapper> const &sharedWrapper
      )
    {
      return new ReduceWrapper( FABRIC_GC_OBJECT_MY_CLASS, inputWrapper, operatorWrapper, sharedWrapper );
    }
    
    ReduceWrapper::ReduceWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<ArrayProducerWrapper> const &inputWrapper,
      RC::ConstHandle<KLC::ReduceOperatorWrapper> const &operatorWrapper,
      RC::ConstHandle<ValueProducerWrapper> const &sharedWrapper
      )
      : ValueProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_inputWrapper( inputWrapper )
      , m_operatorWrapper( operatorWrapper )
      , m_sharedWrapper( sharedWrapper )
      , m_unwrapped(
        Reduce::Create(
          inputWrapper->getUnwrapped(),
          operatorWrapper->getUnwrapped(),
          sharedWrapper? sharedWrapper->getUnwrapped(): RC::ConstHandle<ValueProducer>()
        )
      )
    {
    }
      
    RC::ConstHandle<ValueProducer> ReduceWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ReduceWrapper::getKind() const
    {
      return "Reduce";
    }
    
    void ReduceWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
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
