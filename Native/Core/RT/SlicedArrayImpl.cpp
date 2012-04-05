/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/SlicedArrayImpl.h>

#include <Fabric/Core/RT/VariableArrayImpl.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace RT
  {
    SlicedArrayImpl::SlicedArrayImpl( std::string const &codeName, RC::ConstHandle<Impl> const &memberImpl )
      : ArrayImpl( memberImpl )
      , m_memberImpl( memberImpl )
      , m_memberSize( memberImpl->getAllocSize() )
      , m_variableArrayImpl( memberImpl->getVariableArrayImpl() )
    {
      size_t allocSize = sizeof(bits_t) + m_variableArrayImpl->getAllocSize();
      size_t flags = 0;
      if ( m_memberImpl->isNoAliasUnsafe() )  
        flags |= FlagNoAliasUnsafe;
      if ( m_memberImpl->isExportable() )  
        flags |= FlagExportable;
      initialize( codeName, DT_SLICED_ARRAY, allocSize, flags );

      m_defaultData = malloc( allocSize );
      initializeData( 0, m_defaultData );
    }

    SlicedArrayImpl::~SlicedArrayImpl()
    {
      free( m_defaultData );
    }
    
    void const *SlicedArrayImpl::getDefaultData() const
    {
      return m_defaultData;
    }

    void SlicedArrayImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );

      m_variableArrayImpl->setDatas( count, &reinterpret_cast<bits_t const *>(src)->va, srcStride, &reinterpret_cast<bits_t *>(dst)->va, dstStride );

      uint8_t * const dstEnd = dst + count * dstStride;
      while ( dst != dstEnd )
      {
        bits_t const *srcBits = reinterpret_cast<bits_t const *>(src);
        bits_t *dstBits = reinterpret_cast<bits_t *>(dst);
        dstBits->offset = srcBits->offset;
        dstBits->size = srcBits->size;
        src += srcStride;
        dst += dstStride;
      }
    }

    void SlicedArrayImpl::initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( dst );

      m_variableArrayImpl->initializeDatas(
        count,
        src? &reinterpret_cast<bits_t const *>(src)->va: 0,
        srcStride,
        &reinterpret_cast<bits_t *>(dst)->va,
        dstStride
        );

      uint8_t * const dstEnd = dst + count * dstStride;
      while ( dst != dstEnd )
      {
        bits_t *dstBits = reinterpret_cast<bits_t *>(dst);
        if ( src )
        {
          bits_t const *srcBits = reinterpret_cast<bits_t const *>(src);
          dstBits->offset = srcBits->offset;
          dstBits->size = srcBits->size;
          src += srcStride;
        }
        else dstBits->offset = dstBits->size = 0;
        dst += dstStride;
      }
    }

    void SlicedArrayImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      bits_t const *bits = reinterpret_cast<bits_t const *>(data);
      JSON::ArrayEncoder jsonArrayEncoder = encoder.makeArray();
      for ( size_t i=0; i<bits->size; ++i )
      {
        JSON::Encoder encoder = jsonArrayEncoder.makeElement();
        m_memberImpl->encodeJSON( m_variableArrayImpl->getImmutableMemberData_NoCheck( &bits->va, bits->offset + i ), encoder );
      }
    }
    
    void SlicedArrayImpl::decodeJSON( JSON::Entity const &entity, void *data ) const
    {
      entity.requireArray();
        
      bits_t *dstBits = reinterpret_cast<bits_t *>(data);
      if ( entity.value.array.size != dstBits->size )
        throw Exception( "JSON array size must equal sliced array size" );
        
      size_t membersFound = 0;
      JSON::ArrayDecoder arrayDecoder( entity );
      JSON::Entity elementEntity;
      while ( arrayDecoder.getNext( elementEntity ) )
      {
        FABRIC_ASSERT( membersFound < entity.value.array.size );
        try
        {
          void *memberData = (void*)m_variableArrayImpl->getImmutableMemberData_NoCheck( &dstBits->va, dstBits->offset + membersFound );
          m_memberImpl->decodeJSON( elementEntity, memberData );
        }
        catch ( Exception e )
        {
          throw _(membersFound) + ": " + e;
        }
        ++membersFound;
      }
      
      FABRIC_ASSERT( membersFound == entity.value.array.size );
    }

    void SlicedArrayImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
      m_variableArrayImpl->disposeDatas( count, &reinterpret_cast<bits_t *>(data)->va, stride );
    }
    
    std::string SlicedArrayImpl::descData( void const *data ) const
    {
      bits_t const *srcBits = reinterpret_cast<bits_t const *>( data );

      size_t numMembers = srcBits->size;
      size_t numMembersToDisplay = numMembers;
      if ( numMembersToDisplay > 16 )
        numMembersToDisplay = 16;

      std::string result = "[";
      for ( size_t i=0; i<numMembersToDisplay; ++i )
      {
        if ( result.length() > 1 )
          result += ",";
        result += getMemberImpl()->descData( m_variableArrayImpl->getImmutableMemberData_NoCheck( &srcBits->va, srcBits->offset + i ) );
      }
      if ( numMembers > numMembersToDisplay )
        result += "...";
      result += "]";
      return result;
    }

    bool SlicedArrayImpl::isEquivalentTo( RC::ConstHandle<Impl> const &that ) const
    {
      if ( !isSlicedArray( that->getType() ) )
        return false;
      RC::ConstHandle<SlicedArrayImpl> slicedArrayImpl = RC::ConstHandle<SlicedArrayImpl>::StaticCast( that );

      return getMemberImpl()->isEquivalentTo( slicedArrayImpl->getMemberImpl() );
    }

    size_t SlicedArrayImpl::getNumMembers( void const *data ) const
    {
      FABRIC_ASSERT( data );
      bits_t const *srcBits = static_cast<bits_t const *>(data);
      return srcBits->size;
    }
    
    void const *SlicedArrayImpl::getImmutableMemberData( void const *data, size_t index ) const
    { 
      bits_t const *srcBits = static_cast<bits_t const *>(data);
      if ( index >= srcBits->size )
        throw Exception( "index ("+_(index)+") out of range ("+_(srcBits->size)+")" );
      return m_variableArrayImpl->getImmutableMemberData_NoCheck( &srcBits->va, srcBits->offset + index );
    }
    
    void *SlicedArrayImpl::getMutableMemberData( void *data, size_t index ) const
    { 
      bits_t *srcBits = static_cast<bits_t *>(data);
      if ( index >= srcBits->size )
        throw Exception( "index ("+_(index)+") out of range ("+_(srcBits->size)+")" );
      return m_variableArrayImpl->getMutableMemberData_NoCheck( &srcBits->va, srcBits->offset + index );
    }

    size_t SlicedArrayImpl::getOffset( void const *data ) const
    {
      bits_t const *bits = static_cast<bits_t const *>(data);
      return bits->offset;
    }
    
    size_t SlicedArrayImpl::getSize( void const *data ) const
    {
      bits_t const *bits = static_cast<bits_t const *>(data);
      return bits->size;
    }
    
    void SlicedArrayImpl::setNumMembers( void *data, size_t numMembers, void const *defaultMemberData ) const
    {
      bits_t *bits = static_cast<bits_t *>(data);
      FABRIC_ASSERT( bits->offset == 0 );
      FABRIC_ASSERT( bits->size == m_variableArrayImpl->getNumMembers( &bits->va ) );
      m_variableArrayImpl->setNumMembers( &bits->va, numMembers, defaultMemberData );
      bits->size = numMembers;
    }

    size_t SlicedArrayImpl::getIndirectMemoryUsage( void const *data ) const
    {
      bits_t const *bits = static_cast<bits_t const *>(data);
      return m_variableArrayImpl->getIndirectMemoryUsage( &bits->va );
    }
  }
}
