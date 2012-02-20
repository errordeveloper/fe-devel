/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/MR/ArrayGenerator.h>
#include <Fabric/Core/MR/ArrayOutputOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ArrayGenerator> ArrayGenerator::Create(
      RC::ConstHandle<ValueProducer> const &countValueProducer,
      RC::ConstHandle<ArrayOutputOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &sharedValueProducer
      )
    {
      return new ArrayGenerator( countValueProducer, operator_, sharedValueProducer );
    }
    
    ArrayGenerator::ArrayGenerator(
      RC::ConstHandle<ValueProducer> const &countValueProducer,
      RC::ConstHandle<ArrayOutputOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &sharedValueProducer
      )
      : m_countValueProducer( countValueProducer )
      , m_operator( operator_ )
      , m_sharedValueProducer( sharedValueProducer )
    {
      RC::ConstHandle<RT::Desc> countValueDesc = countValueProducer->getValueDesc();
      if ( !countValueDesc )
        throw Exception("count is invalid");
      if ( !RT::isInteger( countValueDesc->getType() )
        || countValueDesc->getAllocSize() != sizeof(size_t) )
        throw Exception(
          "count value producer type ("
          + _(countValueDesc->getUserName())
          + ") must be 'Size')"
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
    
    RC::ConstHandle<RT::Desc> ArrayGenerator::getElementDesc() const
    {
      return m_operator->getValueDesc();
    }
      
    const RC::Handle<ArrayProducer::ComputeState> ArrayGenerator::createComputeState() const
    {
      return ComputeState::Create( this );
    }

    void ArrayGenerator::flush()
    {
    }
    
    RC::Handle<ArrayGenerator::ComputeState> ArrayGenerator::ComputeState::Create( RC::ConstHandle<ArrayGenerator> const &arrayGenerator )
    {
      return new ComputeState( arrayGenerator );
    }
    
    ArrayGenerator::ComputeState::ComputeState( RC::ConstHandle<ArrayGenerator> const &arrayGenerator )
      : ArrayProducer::ComputeState( arrayGenerator )
      , m_arrayGenerator( arrayGenerator )
    {
      size_t count;
      m_arrayGenerator->m_countValueProducer->createComputeState()->produce( &count );
      setCount( count );
      
      if ( m_arrayGenerator->m_operator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_arrayGenerator->m_sharedValueProducer;
        m_sharedData.resize( sharedValueProducer->getValueDesc()->getAllocSize(), 0 );
        sharedValueProducer->createComputeState()->produce( &m_sharedData[0] );
      }
    }

    ArrayGenerator::ComputeState::~ComputeState()
    {
      if ( m_arrayGenerator->m_operator->takesSharedValue() )
      {
        RC::ConstHandle<ValueProducer> sharedValueProducer = m_arrayGenerator->m_sharedValueProducer;
        sharedValueProducer->getValueDesc()->disposeData( &m_sharedData[0] );
      }
    }
    
    void ArrayGenerator::ComputeState::produce( size_t index, void *data ) const
    {
      if ( index >= getCount() )
        throw Exception( "produce index out of range" );

      RC::ConstHandle<ArrayOutputOperator> operator_ = m_arrayGenerator->m_operator;
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
