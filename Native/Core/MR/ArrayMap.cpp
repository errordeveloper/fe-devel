/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayMap.h>
#include <Fabric/Core/MR/ArrayProducer.h>
#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Core/MR/ArrayIOOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ArrayMap> ArrayMap::Create(
      RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
      RC::ConstHandle<ArrayIOOperator> const &arrayIOOperator,
      RC::ConstHandle<ValueProducer> const &sharedValueProducer
      )
    {
      return new ArrayMap( inputArrayProducer, arrayIOOperator, sharedValueProducer );
    }
    
    ArrayMap::ArrayMap(
      RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
      RC::ConstHandle<ArrayIOOperator> const &arrayIOOperator,
      RC::ConstHandle<ValueProducer> const &sharedValueProducer
      )
      : m_inputArrayProducer( inputArrayProducer )
      , m_arrayIOOperator( arrayIOOperator )
      , m_sharedValueProducer( sharedValueProducer )
    {
      RC::ConstHandle<RT::Desc> inputArrayProducerElementDesc = inputArrayProducer->getElementDesc();
      if ( !inputArrayProducerElementDesc )
        throw Exception("input array producer is invalid");
      RC::ConstHandle<RT::Desc> mapOperatorInputDesc = arrayIOOperator->getInputDesc();
      if ( !mapOperatorInputDesc )
        throw Exception("operator is invalid");
      if ( !mapOperatorInputDesc->isEquivalentTo( inputArrayProducerElementDesc ) )
        throw Exception(
          "input element type ("
          + _(inputArrayProducerElementDesc->getUserName())
          + ") is not equivalent to operator input type ("
          + _(mapOperatorInputDesc->getUserName()) + ")"
          );
      RC::ConstHandle<RT::Desc> reduceOperatorSharedDesc = arrayIOOperator->getSharedDesc();
      if ( reduceOperatorSharedDesc )
      {
        RC::ConstHandle<RT::Desc> sharedValueProducerValueDesc = sharedValueProducer->getValueDesc();
        if ( !sharedValueProducerValueDesc )
          throw Exception( "map operator requires a shared value but no shared value producer is provided" );
        if ( !sharedValueProducerValueDesc->isEquivalentTo( reduceOperatorSharedDesc ) )
          throw Exception(
            "shared value type ("
            + _(sharedValueProducerValueDesc->getUserName())
            + ") is not equivalent to map operator shared type ("
            + _(reduceOperatorSharedDesc->getUserName()) + ")"
            );
      }
    }
    
    RC::ConstHandle<RT::Desc> ArrayMap::getElementDesc() const
    {
      return m_arrayIOOperator->getOutputDesc();
    }

    const RC::Handle<ArrayProducer::ComputeState> ArrayMap::createComputeState() const
    {
      return ComputeState::Create( this );
    }

    void ArrayMap::flush()
    {
      (const_cast<ArrayProducer *>(m_inputArrayProducer.ptr()))->flush();
    }
    
    RC::Handle<ArrayMap::ComputeState> ArrayMap::ComputeState::Create( RC::ConstHandle<ArrayMap> const &arrayMap )
    {
      return new ComputeState( arrayMap );
    }
    
    ArrayMap::ComputeState::ComputeState( RC::ConstHandle<ArrayMap> const &arrayMap )
      : ArrayProducer::ComputeState( arrayMap )
      , m_arrayMap( arrayMap )
      , m_inputArrayProducerComputeState( arrayMap->m_inputArrayProducer->createComputeState() )
    {
      setCount( m_inputArrayProducerComputeState->getCount() );
      
      if ( m_arrayMap->m_arrayIOOperator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_arrayMap->m_sharedValueProducer;
        m_sharedData.resize( sharedValueProducer->getValueDesc()->getAllocSize(), 0 );
        sharedValueProducer->createComputeState()->produce( &m_sharedData[0] );
      }
    }
    
    ArrayMap::ComputeState::~ComputeState()
    {
      if ( m_arrayMap->m_arrayIOOperator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_arrayMap->m_sharedValueProducer;
        sharedValueProducer->getValueDesc()->disposeData( &m_sharedData[0] );
      }
    }
      
    void ArrayMap::ComputeState::produce( size_t index, void *data ) const
    {
      RC::ConstHandle<RT::Desc> inputElementDesc = m_arrayMap->m_inputArrayProducer->getElementDesc();
      
      size_t elementSize = inputElementDesc->getAllocSize();
      void *inputData = alloca( elementSize );
      memset( inputData, 0, elementSize );
      m_inputArrayProducerComputeState->produce( index, inputData );
      
      RC::ConstHandle<ArrayIOOperator> arrayIOOperator = m_arrayMap->m_arrayIOOperator;
      if ( arrayIOOperator->takesIndex() )
      {
        if ( arrayIOOperator->takesCount() )
        {
          size_t count = getCount();
          
          if ( arrayIOOperator->takesSharedValue() )
            arrayIOOperator->call( inputData, data, index, count, &m_sharedData[0] );
          else arrayIOOperator->call( inputData, data, index, count );
        }
        else arrayIOOperator->call( inputData, data, index );
      }
      else arrayIOOperator->call( inputData, data );
      
      inputElementDesc->disposeData( inputData );
    }
  };
};
