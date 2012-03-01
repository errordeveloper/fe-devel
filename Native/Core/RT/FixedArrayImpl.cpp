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
      : ArrayImpl( codeName, DT_FIXED_ARRAY, memberImpl )
      , m_memberImpl( memberImpl )
      , m_memberSize( memberImpl->getAllocSize() )
      , m_length( length )
    {
      setSize( m_memberSize * m_length );

      m_defaultData = malloc( getAllocSize() );
      memset( m_defaultData, 0, getAllocSize() );
      void const *memberDefaultData = m_memberImpl->getDefaultData();
      for ( size_t i=0; i<m_length; ++i )
        m_memberImpl->setData( memberDefaultData, getMutableMemberData( m_defaultData, i ) );
    }
    
    FixedArrayImpl::~FixedArrayImpl()
    {
      disposeData( m_defaultData );
      free( m_defaultData );
    }
    
    void const *FixedArrayImpl::getDefaultData() const
    {
      return m_defaultData;
    }

    void FixedArrayImpl::setData( void const *src, void *dst ) const
    {
      if ( !isMemberShallow() )
      {
        for ( size_t i=0; i<m_length; ++i )
          m_memberImpl->setData( getImmutableMemberData_NoCheck( src, i ), getMutableMemberData_NoCheck( dst, i ) );
      }
      else memcpy( dst, src, getAllocSize() );
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

    void FixedArrayImpl::disposeDatasImpl( void *data, size_t count, size_t stride ) const
    {
      uint8_t *fixedArrayData = static_cast<uint8_t *>( data );
      uint8_t * const fixedArrayDataEnd = fixedArrayData + count * stride;
      while ( fixedArrayData != fixedArrayDataEnd )
      {
        void *memberData = getMutableMemberData_NoCheck( fixedArrayData, 0 );
        m_memberImpl->disposeDatas( memberData, m_length, m_memberImpl->getAllocSize() );
        fixedArrayData += stride;
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
    
    bool FixedArrayImpl::isShallow() const
    {
      return isMemberShallow();
    }

    bool FixedArrayImpl::isNoAliasSafe() const
    {
      return isMemberNoAliasSafe();
    }

    size_t FixedArrayImpl::getIndirectMemoryUsage( void const *data ) const
    {
      size_t total = 0;
      for ( size_t i=0; i<m_length; ++i )
        total += m_memberImpl->getIndirectMemoryUsage( getImmutableMemberData_NoCheck( data, i ) );
      return total;
    }
    
    bool FixedArrayImpl::isExportable() const
    {
      return m_memberImpl->isExportable();
    }
  }
}
