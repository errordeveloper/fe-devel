/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueTransform.h>
#include <Fabric/Core/MR/ValueOutputOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ValueTransform> ValueTransform::Create(
      RC::ConstHandle<ValueProducer> const &input,
      RC::ConstHandle<ValueOutputOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &shared
      )
    {
      return new ValueTransform( input, operator_, shared );
    }
    
    ValueTransform::ValueTransform(
      RC::ConstHandle<ValueProducer> const &input,
      RC::ConstHandle<ValueOutputOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &shared
      )
      : m_input( input )
      , m_operator( operator_ )
      , m_shared( shared )
    {
      RC::ConstHandle<RT::Desc> inputValueDesc = m_input->getValueDesc();
      if ( !inputValueDesc )
        throw Exception("input is invalid");
      RC::ConstHandle<RT::Desc> operatorValueDesc = m_operator->getValueDesc();
      if ( !operatorValueDesc )
        throw Exception("operator is invalid");
      if ( !operatorValueDesc->isEquivalentTo( inputValueDesc ) )
        throw Exception(
          "input value type ("
          + _(inputValueDesc->getUserName())
          + ") is not equivalent to operator value type ("
          + _(operatorValueDesc->getUserName()) + ")"
          );
      m_valueDesc = inputValueDesc;
      
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
    
    RC::ConstHandle<RT::Desc> ValueTransform::getValueDesc() const
    {
      return m_valueDesc;
    }
      
    const RC::Handle<ValueProducer::ComputeState> ValueTransform::createComputeState() const
    {
      return ComputeState::Create( this );
    }
    
    RC::Handle<ValueTransform::ComputeState> ValueTransform::ComputeState::Create( RC::ConstHandle<ValueTransform> const &valueTransform )
    {
      return new ComputeState( valueTransform );
    }

    void ValueTransform::flush()
    {
      (const_cast<ValueProducer *>(m_input.ptr()))->flush();
    }
    
    ValueTransform::ComputeState::ComputeState( RC::ConstHandle<ValueTransform> const &valueTransform )
      : ValueProducer::ComputeState( valueTransform )
      , m_valueTransform( valueTransform )
      , m_inputValueProducerComputeState( valueTransform->m_input->createComputeState() )
    {
      if ( m_valueTransform->m_operator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_valueTransform->m_shared;
        m_sharedData.resize( sharedValueProducer->getValueDesc()->getAllocSize(), 0 );
        sharedValueProducer->createComputeState()->produce( &m_sharedData[0] );
      }
    }
    
    ValueTransform::ComputeState::~ComputeState()
    {
      if ( m_valueTransform->m_operator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_valueTransform->m_shared;
        sharedValueProducer->getValueDesc()->disposeData( &m_sharedData[0] );
      }
    }
    
    void ValueTransform::ComputeState::produce( void *data ) const
    {
      m_inputValueProducerComputeState->produce( data );
      
      RC::ConstHandle<ValueOutputOperator> valueTransformOperator = m_valueTransform->m_operator;
      if ( valueTransformOperator->takesSharedValue() )
        valueTransformOperator->call( data, &m_sharedData[0] );
      else valueTransformOperator->call( data );
    }
  }
}
