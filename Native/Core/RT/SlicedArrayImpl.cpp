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
      : ArrayImpl( codeName, DT_SLICED_ARRAY, memberImpl )
      , m_memberImpl( memberImpl )
      , m_memberSize( memberImpl->getAllocSize() )
      , m_variableArrayImpl( memberImpl->getVariableArrayImpl() )
    {
      setSize( sizeof(bits_t) );
    }
    
    void const *SlicedArrayImpl::getDefaultData() const
    {
      static bits_t defaultData = { 0, 0, 0 };
      return &defaultData;
    }

    void SlicedArrayImpl::setData( void const *src, void *dst ) const
    {
      bits_t const *srcBits = reinterpret_cast<bits_t const *>(src);
      bits_t *dstBits = reinterpret_cast<bits_t *>(dst);
      dstBits->offset = srcBits->offset;
      dstBits->size = srcBits->size;
      if ( dstBits->rcva )
      {
        if ( --dstBits->rcva->refCount == 0 )
        {
          m_variableArrayImpl->disposeData( &dstBits->rcva->varArray );
          free( dstBits->rcva );
        }
      }
      dstBits->rcva = srcBits->rcva;
      if ( dstBits->rcva )
        ++dstBits->rcva->refCount;
    }

    void SlicedArrayImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      bits_t const *bits = reinterpret_cast<bits_t const *>(data);
      JSON::ArrayEncoder jsonArrayEncoder = encoder.makeArray();
      for ( size_t i=0; i<bits->size; ++i )
      {
        JSON::Encoder encoder = jsonArrayEncoder.makeElement();
        m_memberImpl->encodeJSON( m_variableArrayImpl->getImmutableMemberData_NoCheck( &bits->rcva->varArray, bits->offset + i ), encoder );
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
          void *memberData = (void*)m_variableArrayImpl->getImmutableMemberData_NoCheck( &dstBits->rcva->varArray, dstBits->offset + membersFound );
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

    void SlicedArrayImpl::disposeDatasImpl( void *data, size_t count, size_t stride ) const
    {
      bits_t *bits = reinterpret_cast<bits_t *>(data);
      if ( bits->rcva )
      {
        if ( --bits->rcva->refCount == 0 )
        {
          m_variableArrayImpl->disposeData( &bits->rcva->varArray );
          free( bits->rcva );
        }
      }
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
        result += getMemberImpl()->descData( m_variableArrayImpl->getImmutableMemberData_NoCheck( &srcBits->rcva->varArray, srcBits->offset + i ) );
      }
      if ( numMembers > numMembersToDisplay )
        result += "...";
      result += "]";
      return result;
    }
    
    bool SlicedArrayImpl::isShallow() const
    {
      return false;
    }

    bool SlicedArrayImpl::isNoAliasSafe() const
    {
      return isMemberNoAliasSafe();
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
      bits_t const *srcBits = reinterpret_cast<bits_t const *>(data);
      return srcBits->size;
    }
    
    void const *SlicedArrayImpl::getImmutableMemberData( void const *data, size_t index ) const
    { 
      bits_t const *srcBits = reinterpret_cast<bits_t const *>(data);
      if ( index >= srcBits->size )
        throw Exception( "index ("+_(index)+") out of range ("+_(srcBits->size)+")" );
      return m_variableArrayImpl->getImmutableMemberData_NoCheck( &srcBits->rcva->varArray, srcBits->offset + index );
    }
    
    void *SlicedArrayImpl::getMutableMemberData( void *data, size_t index ) const
    { 
      bits_t *srcBits = reinterpret_cast<bits_t *>(data);
      if ( index >= srcBits->size )
        throw Exception( "index ("+_(index)+") out of range ("+_(srcBits->size)+")" );
      return m_variableArrayImpl->getMutableMemberData_NoCheck( &srcBits->rcva->varArray, srcBits->offset + index );
    }

    size_t SlicedArrayImpl::getOffset( void const *data ) const
    {
      bits_t const *bits = reinterpret_cast<bits_t const *>(data);
      return bits->offset;
    }
    
    size_t SlicedArrayImpl::getSize( void const *data ) const
    {
      bits_t const *bits = reinterpret_cast<bits_t const *>(data);
      return bits->size;
    }
    
    void SlicedArrayImpl::setNumMembers( void *data, size_t numMembers, void const *defaultMemberData ) const
    {
      bits_t *bits = reinterpret_cast<bits_t *>(data);
      FABRIC_ASSERT( bits->offset == 0 );
      FABRIC_ASSERT( bits->rcva == 0 || bits->size == m_variableArrayImpl->getNumMembers( &bits->rcva->varArray ) );
      if ( !bits->rcva )
      {
        bits->rcva = static_cast<ref_counted_va_t *>( malloc( sizeof( ref_counted_va_t ) ) );
        bits->rcva->refCount = 1;
        memset( &bits->rcva->varArray, 0, sizeof( bits->rcva->varArray ) );
      }
      m_variableArrayImpl->setNumMembers( &bits->rcva->varArray, numMembers, defaultMemberData );
      bits->size = numMembers;
    }

    size_t SlicedArrayImpl::getIndirectMemoryUsage( void const *data ) const
    {
      bits_t const *bits = reinterpret_cast<bits_t const *>(data);
      if ( bits->rcva )
        return sizeof( *bits->rcva ) + m_variableArrayImpl->getIndirectMemoryUsage( &bits->rcva->varArray );
      else return 0;
    }
    
    bool SlicedArrayImpl::isExportable() const
    {
      return m_variableArrayImpl->isExportable();
    }
  }
}
