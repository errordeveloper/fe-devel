/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ValueCache.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ValueCache> ValueCache::Create(
      RC::ConstHandle<ValueProducer> const &input
      )
    {
      return new ValueCache( input );
    }
    
    ValueCache::ValueCache(
      RC::ConstHandle<ValueProducer> const &input
      )
      : ValueProducer()
      , m_input( input )
      , m_mutex( "ValueCache" )
    {
      RC::ConstHandle<RT::Desc> inputValueDesc = m_input->getValueDesc();
      if ( !inputValueDesc )
        throw Exception("input is invalid");
      m_valueDesc = inputValueDesc;
    }
    
    ValueCache::~ValueCache()
    {
      if ( m_cacheData.size() > 0 )
        m_valueDesc->disposeData( &m_cacheData[0] );
    }
    
    RC::ConstHandle<RT::Desc> ValueCache::getValueDesc() const
    {
      return m_valueDesc;
    }

    const RC::Handle<ValueProducer::ComputeState> ValueCache::createComputeState() const
    {
      return ComputeState::Create( this );
    }

    void ValueCache::flush()
    {
      (const_cast<ValueProducer *>(m_input.ptr()))->flush();

      if ( m_cacheData.size() > 0 )
      {
        Util::Mutex::Lock mutexLock( m_mutex );
        m_valueDesc->disposeData( &m_cacheData[0] );
        m_cacheData.resize(0);
      }
    }
    
    RC::Handle<ValueCache::ComputeState> ValueCache::ComputeState::Create( RC::ConstHandle<ValueCache> const &valueCache )
    {
      return new ComputeState( valueCache );
    }
    
    ValueCache::ComputeState::ComputeState( RC::ConstHandle<ValueCache> const &valueCache )
      : ValueProducer::ComputeState( valueCache )
      , m_valueCache( valueCache )
      , m_inputValueProducerComputeState( valueCache->m_input->createComputeState() )
      , m_mutex( valueCache->m_mutex )
    {
    }
    
    void ValueCache::ComputeState::produce( void *data ) const
    {
      {
        Util::Mutex::Lock mutexLock( m_mutex );
        if ( m_valueCache->m_cacheData.size() > 0 )
        {
          m_valueCache->m_valueDesc->setData( &m_valueCache->m_cacheData[0], data );
          return;
        }
      }

      m_inputValueProducerComputeState->produce( data );

      {
        Util::Mutex::Lock mutexLock( m_mutex );
        m_valueCache->m_cacheData.resize( m_valueCache->m_valueDesc->getAllocSize(), 0 );
        m_valueCache->m_valueDesc->setData( data, &m_valueCache->m_cacheData[0] );
      }
    }
  }
}
