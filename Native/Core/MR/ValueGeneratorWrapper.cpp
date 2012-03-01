/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueGeneratorWrapper.h>
#include <Fabric/Core/MR/ValueGenerator.h>
#include <Fabric/Core/MR/ValueGeneratorOperator.h>
#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Core/KLC/ValueGeneratorOperator.h>
#include <Fabric/Core/KLC/ValueGeneratorOperatorWrapper.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ValueGeneratorWrapper, ValueProducerWrapper );
    
    RC::Handle<ValueGeneratorWrapper> ValueGeneratorWrapper::Create(
      RC::ConstHandle<KLC::ValueGeneratorOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
    {
      return new ValueGeneratorWrapper( FABRIC_GC_OBJECT_MY_CLASS, operator_, sharedValueProducer );
    }
    
    ValueGeneratorWrapper::ValueGeneratorWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<KLC::ValueGeneratorOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
      : ValueProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_operator( operator_ )
      , m_sharedValueProducer( sharedValueProducer )
      , m_unwrapped(
        ValueGenerator::Create(
          operator_->getUnwrapped(),
          sharedValueProducer? sharedValueProducer->getUnwrapped(): RC::ConstHandle<ValueProducer>()
          )
        )
    {
    }
      
    RC::ConstHandle<ValueProducer> ValueGeneratorWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ValueGeneratorWrapper::getKind() const
    {
      return "ValueGenerator";
    }
    
    void ValueGeneratorWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
    {
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
