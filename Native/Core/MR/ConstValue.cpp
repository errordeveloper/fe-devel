/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ConstValue.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ConstValue> ConstValue::Create(
      RC::ConstHandle<RT::Manager> const &rtManager,
      RC::ConstHandle<RT::Desc> const &valueDesc,
      JSON::Entity const &entity
      )
    {
      return new ConstValue( rtManager, valueDesc, entity );
    }
    
    ConstValue::ConstValue(
      RC::ConstHandle<RT::Manager> const &rtManager,
      RC::ConstHandle<RT::Desc> const &valueDesc,
      JSON::Entity const &entity
      )
      : ValueProducer()
      , m_valueDesc( valueDesc )
    {
      m_data.resize( m_valueDesc->getAllocSize(), 0 );
      m_valueDesc->decodeJSON( entity, &m_data[0] );
    }
    
    RC::Handle<ConstValue> ConstValue::Create(
      RC::ConstHandle<RT::Manager> const &rtManager,
      RC::ConstHandle<RT::Desc> const &valueDesc,
      void const *data
      )
    {
      return new ConstValue( rtManager, valueDesc, data );
    }
    
    ConstValue::ConstValue(
      RC::ConstHandle<RT::Manager> const &rtManager,
      RC::ConstHandle<RT::Desc> const &valueDesc,
      void const *data
      )
      : ValueProducer()
      , m_valueDesc( valueDesc )
    {
      m_data.resize( m_valueDesc->getAllocSize(), 0 );
      m_valueDesc->setData( data, &m_data[0] );
    }
    
    ConstValue::~ConstValue()
    {
      m_valueDesc->disposeData( &m_data[0] );
    }
    
    RC::ConstHandle<RT::Desc> ConstValue::getValueDesc() const
    {
      return m_valueDesc;
    }

    const RC::Handle<ValueProducer::ComputeState> ConstValue::createComputeState() const
    {
      return ComputeState::Create( this );
    }
    
    void const *ConstValue::getImmutableData() const
    {
      return &m_data[0];
    }

    void ConstValue::flush()
    {
    }
    
    RC::Handle<ConstValue::ComputeState> ConstValue::ComputeState::Create( RC::ConstHandle<ConstValue> const &constValue )
    {
      return new ComputeState( constValue );
    }
    
    ConstValue::ComputeState::ComputeState( RC::ConstHandle<ConstValue> const &constValue )
      : ValueProducer::ComputeState( constValue )
      , m_constValue( constValue )
    {
    }
    
    ConstValue::ComputeState::~ComputeState()
    {
    }
    
    void ConstValue::ComputeState::produce( void *data ) const
    {
      return m_constValue->m_valueDesc->setData( &m_constValue->m_data[0], data );
    }
    
    void ConstValue::ComputeState::produceJSON( JSON::Encoder &jg ) const
    {
      return m_constValue->m_valueDesc->encodeJSON( &m_constValue->m_data[0], jg );
    }
  }
}
