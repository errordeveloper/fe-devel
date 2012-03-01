/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueGenerator.h>
#include <Fabric/Core/MR/ValueOutputOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ValueGenerator> ValueGenerator::Create(
      RC::ConstHandle<ValueOutputOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &shared
      )
    {
      return new ValueGenerator( operator_, shared );
    }
    
    ValueGenerator::ValueGenerator(
      RC::ConstHandle<ValueOutputOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &shared
      )
      : m_operator( operator_ )
      , m_shared( shared )
    {
      RC::ConstHandle<RT::Desc> operatorValueDesc = m_operator->getValueDesc();
      if ( !operatorValueDesc )
        throw Exception("operator is invalid");
      m_valueDesc = operatorValueDesc;
      
      RC::ConstHandle<RT::Desc> operatorSharedDesc = m_operator->getSharedDesc();
      if ( operatorSharedDesc )
      {
        RC::ConstHandle<RT::Desc> sharedValueDesc = m_shared? m_shared->getValueDesc(): RC::ConstHandle<RT::Desc>();
        if ( !sharedValueDesc )
          throw Exception( "operator requires a shared value but no shared value producer is provided" );
        if ( !sharedValueDesc->isEquivalentTo( operatorSharedDesc ) )
          throw Exception(
            "shared value type ("
            + _(sharedValueDesc->getUserName())
            + ") is not equivalent to operator shared type ("
            + _(operatorSharedDesc->getUserName()) + ")"
            );
      }
    }
    
    RC::ConstHandle<RT::Desc> ValueGenerator::getValueDesc() const
    {
      return m_valueDesc;
    }
      
    const RC::Handle<ValueProducer::ComputeState> ValueGenerator::createComputeState() const
    {
      return ComputeState::Create( this );
    }

    void ValueGenerator::flush()
    {
    }
    
    RC::Handle<ValueGenerator::ComputeState> ValueGenerator::ComputeState::Create( RC::ConstHandle<ValueGenerator> const &valueTransform )
    {
      return new ComputeState( valueTransform );
    }
    
    ValueGenerator::ComputeState::ComputeState( RC::ConstHandle<ValueGenerator> const &valueTransform )
      : ValueProducer::ComputeState( valueTransform )
      , m_valueTransform( valueTransform )
    {
      if ( m_valueTransform->m_operator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_valueTransform->m_shared;
        m_sharedData.resize( sharedValueProducer->getValueDesc()->getAllocSize(), 0 );
        sharedValueProducer->createComputeState()->produce( &m_sharedData[0] );
      }
    }
    
    ValueGenerator::ComputeState::~ComputeState()
    {
      if ( m_valueTransform->m_operator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_valueTransform->m_shared;
        sharedValueProducer->getValueDesc()->disposeData( &m_sharedData[0] );
      }
    }
    
    void ValueGenerator::ComputeState::produce( void *data ) const
    {
      RC::ConstHandle<ValueOutputOperator> valueTransformOperator = m_valueTransform->m_operator;
      if ( valueTransformOperator->takesSharedValue() )
        valueTransformOperator->call( data, &m_sharedData[0] );
      else valueTransformOperator->call( data );
    }
  }
}
