/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "FixedArrayImpl.h"

#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace RT
  {
    FixedArrayImpl::FixedArrayImpl( std::string const &codeName, RC::ConstHandle<Impl> const &memberImpl, size_t length )
      : ArrayImpl( memberImpl )
      , m_memberImpl( memberImpl )
      , m_memberSize( memberImpl->getAllocSize() )
      , m_length( length )
    {
      size_t flags = 0;
      if ( m_memberImpl->isShallow() )
        flags |= FlagShallow;
      if ( m_memberImpl->isNoAliasUnsafe() )
        flags |= FlagNoAliasUnsafe;
      if ( m_memberImpl->isExportable() )
        flags |= FlagExportable;
      initialize( codeName, DT_FIXED_ARRAY, m_memberSize * m_length, flags );

      m_defaultData = malloc( getAllocSize() );
      m_memberImpl->initializeDatas( m_length, m_memberImpl->getDefaultData(), 0, m_defaultData, m_memberSize );
    }
    
    FixedArrayImpl::~FixedArrayImpl()
    {
      m_memberImpl->disposeDatas( m_length, m_defaultData, m_memberSize );
      free( m_defaultData );
    }
    
    void const *FixedArrayImpl::getDefaultData() const
    {
      return m_defaultData;
    }

    void FixedArrayImpl::initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;

      while ( dst != dstEnd )
      {
        m_memberImpl->initializeDatas( m_length, src, m_memberSize, dst, m_memberSize );
        if ( src )
          src += srcStride;
        dst += dstStride;
      }
    }

    void FixedArrayImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;

      while ( dst != dstEnd )
      {
        m_memberImpl->setDatas( m_length, src, m_memberSize, dst, m_memberSize );
        src += srcStride;
        dst += dstStride;
      }
    }
    
    void FixedArrayImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      JSON::ArrayEncoder arrayEncoder = encoder.makeArray();
      for ( size_t i = 0; i < m_length; ++i )
      {
        void const *memberData = getImmutableMemberData_NoCheck( data, i );
        JSON::Encoder elementEncoder = arrayEncoder.makeElement();
        m_memberImpl->encodeJSON( memberData, elementEncoder );
      }
    }
    
    void FixedArrayImpl::decodeJSON( JSON::Entity const &entity, void *data ) const
    {
      entity.requireArray();
      if ( entity.arraySize() != m_length )
        throw Exception( "JSON array is of wrong size (expected " + _(m_length) + ", actual " + _(entity.value.array.size) + ")" );
        
      size_t membersFound = 0;
      JSON::ArrayDecoder arrayDecoder( entity );
      JSON::Entity elementEntity;
      while ( arrayDecoder.getNext( elementEntity ) )
      {
        FABRIC_ASSERT( membersFound < m_length );
        try
        {
          void *memberData = getMutableMemberData_NoCheck( data, membersFound );
          m_memberImpl->decodeJSON( elementEntity, memberData );
        }
        catch ( Exception e )
        {
          throw _(membersFound) + ": " + e;
        }
        ++membersFound;
      }
      
      FABRIC_ASSERT( membersFound == m_length );
    }

    void FixedArrayImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
      size_t memberAllocSize = m_memberImpl->getAllocSize();
      uint8_t * const dataEnd = data + count * stride;
      while ( data != dataEnd )
      {
        void *memberData = getMutableMemberData_NoCheck( data, 0 );
        m_memberImpl->disposeDatas( m_length, memberData, memberAllocSize );
        data += stride;
      }
    }
    
    bool FixedArrayImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      if ( !isFixedArray( impl->getType() ) )
        return false;
      RC::ConstHandle<FixedArrayImpl> fixedArrayImpl = RC::ConstHandle<FixedArrayImpl>::StaticCast( impl );
      
      return m_length == fixedArrayImpl->m_length
        && m_memberImpl->isEquivalentTo( fixedArrayImpl->m_memberImpl );
    }
    
    size_t FixedArrayImpl::getNumMembers( void const *data ) const
    {
      return m_length;
    }
          
    void const *FixedArrayImpl::getImmutableMemberData( void const *data, size_t index ) const
    { 
      if ( index >= m_length )
        throw Exception( "index out of range" );
      return getImmutableMemberData_NoCheck( data, index );
    }
    
    void *FixedArrayImpl::getMutableMemberData( void *data, size_t index ) const
    { 
      if ( index >= m_length )
        throw Exception( "index out of range" );
      return getMutableMemberData_NoCheck( data, index );
    }
    
    size_t FixedArrayImpl::getNumMembers() const
    {
      return m_length;
    }

    size_t FixedArrayImpl::getIndirectMemoryUsage( void const *data ) const
    {
      size_t total = 0;
      for ( size_t i=0; i<m_length; ++i )
        total += m_memberImpl->getIndirectMemoryUsage( getImmutableMemberData_NoCheck( data, i ) );
      return total;
    }
  }
}
