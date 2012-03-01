/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayGeneratorWrapper.h>
#include <Fabric/Core/MR/ArrayGenerator.h>
#include <Fabric/Core/MR/ArrayGeneratorOperator.h>
#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Core/MR/ArrayProducerWrapper.h>
#include <Fabric/Core/KLC/ArrayGeneratorOperator.h>
#include <Fabric/Core/KLC/ArrayGeneratorOperatorWrapper.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ArrayGeneratorWrapper, ArrayProducerWrapper );
    
    RC::Handle<ArrayGeneratorWrapper> ArrayGeneratorWrapper::Create(
      RC::ConstHandle<ValueProducerWrapper> const &countValueProducer,
      RC::ConstHandle<KLC::ArrayGeneratorOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
    {
      return new ArrayGeneratorWrapper( FABRIC_GC_OBJECT_MY_CLASS, countValueProducer, operator_, sharedValueProducer );
    }
    
    ArrayGeneratorWrapper::ArrayGeneratorWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<ValueProducerWrapper> const &countValueProducer,
      RC::ConstHandle<KLC::ArrayGeneratorOperatorWrapper> const &operator_,
      RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
      )
      : ArrayProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_countValueProducer( countValueProducer )
      , m_operator( operator_ )
      , m_sharedValueProducer( sharedValueProducer )
      , m_unwrapped(
        ArrayGenerator::Create(
          countValueProducer->getUnwrapped(),
          operator_->getUnwrapped(),
          sharedValueProducer? sharedValueProducer->getUnwrapped(): RC::ConstHandle<ValueProducer>()
          )
        )
    {
    }
      
    RC::ConstHandle<ArrayProducer> ArrayGeneratorWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ArrayGeneratorWrapper::getKind() const
    {
      return "ArrayGenerator";
    }
    
    void ArrayGeneratorWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
    {
      {
        JSON::Encoder jg = objectEncoder.makeMember( "count" );
        m_countValueProducer->toJSON( jg );
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
