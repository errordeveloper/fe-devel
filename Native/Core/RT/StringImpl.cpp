/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/StringImpl.h>

#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace RT
  {
    StringImpl::StringImpl( std::string const &codeName )
      : ComparableImpl(  )
    {
      initialize( codeName, DT_STRING, sizeof(bits_t *), FlagExportable );
    }
    
    void const *StringImpl::getDefaultData() const
    {
      static bits_t *defaultData = 0;
      return &defaultData;
    }
    
    void StringImpl::initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;
      while ( dst != dstEnd )
      {
        bits_t *&dstBits = *reinterpret_cast<bits_t **>( dst );
        if ( src )
        {
          bits_t *srcBits = *reinterpret_cast<bits_t * const *>( src );
          dstBits = srcBits;
          if ( dstBits )
            dstBits->refCount.increment();
          src += srcStride;
        }
        else dstBits = 0;
        dst += dstStride;
      }
    }
    
    void StringImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;

      while ( dst != dstEnd )
      {
        bits_t *srcBits = *reinterpret_cast<bits_t * const *>( src );
        bits_t *&dstBits = *reinterpret_cast<bits_t **>( dst );
        if ( dstBits != srcBits )
        {
          if ( dstBits && dstBits->refCount.decrementAndGetValue() == 0 )
            free( dstBits );
          
          dstBits = srcBits;
          
          if ( dstBits )
            dstBits->refCount.increment();
        }

        src += srcStride;
        dst += dstStride;
      }
    }
    
    void StringImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      encoder.makeString( getValueData(data), getValueLength(data) );
    }
    
    void StringImpl::decodeJSON( JSON::Entity const &entity, void *dst ) const
    {
      entity.requireString();
      entity.stringGetData( GetMutableValueData( dst, entity.stringLength() ) );
    }

    void StringImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
      uint8_t * const dataEnd = data + count * stride;
      while ( data != dataEnd )
      {
        bits_t *bits = *reinterpret_cast<bits_t **>( data );
        if ( bits && bits->refCount.decrementAndGetValue() == 0 )
          free( bits );
        data += stride;
      }
    }
    
    std::string StringImpl::descData( void const *data ) const
    {
      std::string result = "\"";
      size_t length = getValueLength(data);
      if ( length > 64 )
      {
        result.append( getValueData(data), 64 );
        result += "...";
      }
      else if ( length )
        result += getValueData(data);
      result += "\"";
      return result;
    }
    
    bool StringImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      return isString( impl->getType() );
    }

    size_t StringImpl::hash( void const *data ) const
    {
      // [pzion 20111014] This hash function is referred to as djb2.
      // Thanks Manon!!
      size_t stringLength = getValueLength( data );
      uint8_t const *stringData = reinterpret_cast<uint8_t const *>( getValueData( data ) );
      size_t result = 5381;
      while ( stringLength > 0 )
      {
        result = ((result << 5) + result) ^ uint32_t(*stringData);
        --stringLength;
        ++stringData;
      }
      return result;
    }
    
    int StringImpl::compare( void const *lhsData, void const *rhsData ) const
    {
      char const *lhsStringData = getValueData( lhsData );
      size_t lhsStringLength = getValueLength( lhsData );
      char const *rhsStringData = getValueData( rhsData );
      size_t rhsStringLength = getValueLength( rhsData );
      size_t minStringLength = std::min( lhsStringLength, rhsStringLength );
      int cmp = memcmp( lhsStringData, rhsStringData, minStringLength );
      if ( cmp == 0 )
      {
        if ( lhsStringLength < rhsStringLength )
          return -1;
        else if ( lhsStringLength == rhsStringLength )
          return 0;
        else return 1;
      }
      else return cmp;
    }

    size_t StringImpl::getIndirectMemoryUsage( void const *data ) const
    {
      bits_t const *bits = *static_cast<bits_t const * const *>( data );
      size_t total = 0;
      if ( bits )
        total += sizeof( *bits ) + bits->allocSize;
      return total;
    }
  }
}
