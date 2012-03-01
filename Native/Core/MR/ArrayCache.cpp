/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayCache.h>
#include <Fabric/Core/MR/ArrayOutputOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<ArrayCache> ArrayCache::Create(
      RC::ConstHandle<ArrayProducer> const &inputArrayProducer
      )
    {
      return new ArrayCache( inputArrayProducer );
    }
    
    ArrayCache::ArrayCache(
      RC::ConstHandle<ArrayProducer> const &inputArrayProducer
      )
      : m_inputArrayProducer( inputArrayProducer )
      , m_cacheCountExists( false )
      , m_mutex( "ArrayCache" )
    {
      RC::ConstHandle<RT::Desc> inputElementDesc = inputArrayProducer->getElementDesc();
      if ( !inputElementDesc )
        throw Exception("input is invalid");
      m_inputElementDesc = inputElementDesc;
    }

    ArrayCache::~ArrayCache()
    {
    }
    
    RC::ConstHandle<RT::Desc> ArrayCache::getElementDesc() const
    {
      return m_inputElementDesc;
    }

    const RC::Handle<ArrayProducer::ComputeState> ArrayCache::createComputeState() const
    {
      return ComputeState::Create( this );
    }

    void ArrayCache::flush()
    {
      (const_cast<ArrayProducer *>(m_inputArrayProducer.ptr()))->flush(); 

      if ( m_cacheData.size() > 0 )
      {
        Util::Mutex::Lock mutexLock( m_mutex );
        size_t allocSize = m_inputElementDesc->getAllocSize();
        m_inputElementDesc->disposeDatas( &m_cacheData[0], m_cacheData.size() / allocSize, allocSize );
        m_cacheData.resize(0);
        m_cacheDataExists.resize(0);
        m_cacheCountExists = false;
      }
    }
    
    RC::Handle<ArrayCache::ComputeState> ArrayCache::ComputeState::Create( RC::ConstHandle<ArrayCache> const &arrayCache )
    {
      return new ComputeState( arrayCache );
    }
    
    ArrayCache::ComputeState::ComputeState( RC::ConstHandle<ArrayCache> const &arrayCache )
      : ArrayProducer::ComputeState( arrayCache )
      , m_arrayCache( arrayCache )
      , m_inputArrayProducerComputeState( arrayCache->m_inputArrayProducer->createComputeState() )
      , m_mutex( arrayCache->m_mutex )
    {
      // andrewmacp 2012-01-19:
      // It wouldn't be incorrect to do this without a mutex but it's
      // clearer and simpler to leave one in, we may remove later.
      {
        Util::Mutex::Lock mutexLock( m_arrayCache->m_mutex );
        if ( m_arrayCache->m_cacheCountExists )
        {
          setCount( m_arrayCache->m_cacheCount );
          return;
        }
      }

      size_t count = m_inputArrayProducerComputeState->getCount();
      setCount( count );

      {
        Util::Mutex::Lock mutexLock( m_arrayCache->m_mutex );
        m_arrayCache->m_cacheCount = count;
        m_arrayCache->m_cacheCountExists = true;
      }
    }
    
    void ArrayCache::ComputeState::produce( size_t index, void *data ) const
    {
      {
        Util::Mutex::Lock mutexLock( m_mutex );
        if ( m_arrayCache->m_cacheDataExists.size() > index &&
             m_arrayCache->m_cacheDataExists[ index ] )
        {
          m_arrayCache->m_inputElementDesc->setData( &m_arrayCache->m_cacheData[ index * m_arrayCache->m_inputElementDesc->getAllocSize() ], data );
          return;
        }
      }

      m_inputArrayProducerComputeState->produce( index, data );

      {
        Util::Mutex::Lock mutexLock( m_mutex );
        size_t arraySize = getCount();
        size_t allocSize = m_arrayCache->m_inputElementDesc->getAllocSize();

        if ( m_arrayCache->m_cacheDataExists.size() < arraySize )
        {
          m_arrayCache->m_cacheData.resize( allocSize * arraySize, 0 );
          m_arrayCache->m_cacheDataExists.resize( arraySize, 0 );
        }

        if ( index < arraySize )
        {
          m_arrayCache->m_inputElementDesc->setData( data, &m_arrayCache->m_cacheData[ index * allocSize ] );
          m_arrayCache->m_cacheDataExists[ index ] = true;
        }
      }
    }
  };
};
