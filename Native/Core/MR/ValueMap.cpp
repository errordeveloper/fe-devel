/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueMap.h>
#include <Fabric/Core/MR/ValueMapOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ValueMap> ValueMap::Create(
      RC::ConstHandle<ValueProducer> const &input,
      RC::ConstHandle<ValueIOOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &shared
      )
    {
      return new ValueMap( input, operator_, shared );
    }
    
    ValueMap::ValueMap(
      RC::ConstHandle<ValueProducer> const &input,
      RC::ConstHandle<ValueIOOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &shared
      )
      : m_input( input )
      , m_operator( operator_ )
      , m_shared( shared )
    {
      RC::ConstHandle<RT::Desc> inputValueDesc = input->getValueDesc();
      if ( !inputValueDesc )
        throw Exception("input is invalid");
      RC::ConstHandle<RT::Desc> operatorInputDesc = operator_->getInputDesc();
      if ( !operatorInputDesc )
        throw Exception("operator is invalid");
      if ( !operatorInputDesc->isEquivalentTo( inputValueDesc ) )
        throw Exception(
          "input value type ("
          + _(inputValueDesc->getUserName())
          + ") is not equivalent to value map operator input type ("
          + _(operatorInputDesc->getUserName()) + ")"
          );
      RC::ConstHandle<RT::Desc> operatorSharedDesc = operator_->getSharedDesc();
      if ( operatorSharedDesc )
      {
        RC::ConstHandle<RT::Desc> sharedValueDesc = shared->getValueDesc();
        if ( !sharedValueDesc )
          throw Exception( "operator requires a shared value but no shared value producer is provided" );
        if ( !sharedValueDesc->isEquivalentTo( operatorSharedDesc ) )
          throw Exception(
            "shared value type ("
            + _(sharedValueDesc->getUserName())
            + ") is not equivalent to value map operator shared type ("
            + _(operatorSharedDesc->getUserName()) + ")"
            );
      }
    }
    
    RC::ConstHandle<RT::Desc> ValueMap::getValueDesc() const
    {
      return m_operator->getOutputDesc();
    }
      
    const RC::Handle<ValueProducer::ComputeState> ValueMap::createComputeState() const
    {
      return ComputeState::Create( this );
    }

    void ValueMap::flush()
    {
      (const_cast<ValueProducer *>(m_input.ptr()))->flush();
    }
    
    RC::Handle<ValueMap::ComputeState> ValueMap::ComputeState::Create( RC::ConstHandle<ValueMap> const &valueMap )
    {
      return new ComputeState( valueMap );
    }
    
    ValueMap::ComputeState::ComputeState( RC::ConstHandle<ValueMap> const &valueMap )
      : ValueProducer::ComputeState( valueMap )
      , m_inputComputeState( valueMap->m_input->createComputeState() )
      , m_inputDesc( valueMap->getValueDesc() )
      , m_operator( valueMap->m_operator )
      , m_shared( valueMap->m_shared )
    {
      if ( m_operator->takesSharedValue() )
      {
        m_sharedData.resize( m_shared->getValueDesc()->getAllocSize(), 0 );
        m_shared->createComputeState()->produce( &m_sharedData[0] );
      }
    }
    
    ValueMap::ComputeState::~ComputeState()
    {
      if ( m_operator->takesSharedValue() )
        m_shared->getValueDesc()->disposeData( &m_sharedData[0] );
    }
    
    void ValueMap::ComputeState::produce( void *data ) const
    {
      size_t elementSize = m_inputDesc->getAllocSize();
      void *inputData = alloca( elementSize );
      memset( inputData, 0, elementSize );
      m_inputComputeState->produce( inputData );
      
      if ( m_operator->takesSharedValue() )
        m_operator->call( inputData, data, &m_sharedData[0] );
      else m_operator->call( inputData, data );
      
      m_inputDesc->disposeData( inputData );
    }
  }
}
