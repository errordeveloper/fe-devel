/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayTransform.h>
#include <Fabric/Core/MR/ArrayOutputOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ArrayTransform> ArrayTransform::Create(
      RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
      RC::ConstHandle<ArrayOutputOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &sharedValueProducer
      )
    {
      return new ArrayTransform( inputArrayProducer, operator_, sharedValueProducer );
    }
    
    ArrayTransform::ArrayTransform(
      RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
      RC::ConstHandle<ArrayOutputOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &sharedValueProducer
      )
      : m_inputArrayProducer( inputArrayProducer )
      , m_operator( operator_ )
      , m_sharedValueProducer( sharedValueProducer )
    {
      RC::ConstHandle<RT::Desc> inputElementDesc = inputArrayProducer->getElementDesc();
      if ( !inputElementDesc )
        throw Exception("input is invalid");
      RC::ConstHandle<RT::Desc> operatorValueDesc = m_operator->getValueDesc();
      if ( !operatorValueDesc )
        throw Exception("operator is invalid");
      if ( !operatorValueDesc->isEquivalentTo( inputElementDesc ) )
        throw Exception(
          "input type ("
          + _(inputElementDesc->getUserName())
          + ") is not equivalent to operator value type ("
          + _(operatorValueDesc->getUserName()) + ")"
          );
      RC::ConstHandle<RT::Desc> operatorSharedDesc = m_operator->getSharedDesc();
      if ( operatorSharedDesc )
      {
        RC::ConstHandle<RT::Desc> sharedValueDesc = m_sharedValueProducer->getValueDesc();
        if ( !sharedValueDesc )
          throw Exception( "operator requires a shared value but no shared value producer is provided" );
        if ( !sharedValueDesc->isEquivalentTo( operatorSharedDesc ) )
          throw Exception(
            "shared value type ("
            + _(operatorSharedDesc->getUserName())
            + ") is not equivalent to operator shared type ("
            + _(sharedValueDesc->getUserName()) + ")"
            );
      }
    }
    
    RC::ConstHandle<RT::Desc> ArrayTransform::getElementDesc() const
    {
      return m_operator->getValueDesc();
    }
      
    const RC::Handle<ArrayProducer::ComputeState> ArrayTransform::createComputeState() const
    {
      return ComputeState::Create( this );
    }

    void ArrayTransform::flush()
    {
      (const_cast<ArrayProducer *>(m_inputArrayProducer.ptr()))->flush();
    }
    
    RC::Handle<ArrayTransform::ComputeState> ArrayTransform::ComputeState::Create( RC::ConstHandle<ArrayTransform> const &arrayTransform )
    {
      return new ComputeState( arrayTransform );
    }
    
    ArrayTransform::ComputeState::ComputeState( RC::ConstHandle<ArrayTransform> const &arrayTransform )
      : ArrayProducer::ComputeState( arrayTransform )
      , m_arrayTransform( arrayTransform )
      , m_inputArrayProducerComputeState( arrayTransform->m_inputArrayProducer->createComputeState() )
    {
      setCount( m_inputArrayProducerComputeState->getCount() );
      
      if ( m_arrayTransform->m_operator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_arrayTransform->m_sharedValueProducer;
        m_sharedData.resize( sharedValueProducer->getValueDesc()->getAllocSize(), 0 );
        sharedValueProducer->createComputeState()->produce( &m_sharedData[0] );
      }
    }
    
    ArrayTransform::ComputeState::~ComputeState()
    {
      if ( m_arrayTransform->m_operator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_arrayTransform->m_sharedValueProducer;
        sharedValueProducer->getValueDesc()->disposeData( &m_sharedData[0] );
      }
    }
    
    void ArrayTransform::ComputeState::produce( size_t index, void *data ) const
    {
      m_inputArrayProducerComputeState->produce( index, data );
      
      RC::ConstHandle<ArrayOutputOperator> operator_ = m_arrayTransform->m_operator;
      if ( operator_->takesIndex() )
      {
        if ( operator_->takesCount() )
        {
          size_t count = getCount();
              
          if ( operator_->takesSharedValue() )
            operator_->call( data, index, count, &m_sharedData[0] );
          else operator_->call( data, index, count );
        }
        else operator_->call( data, index );
      }
      else operator_->call( data );
    }
  };
};
