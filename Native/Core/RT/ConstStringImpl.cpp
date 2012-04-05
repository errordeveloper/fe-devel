/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/ConstStringImpl.h>

#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    ConstStringImpl::ConstStringImpl( std::string const &codeName )
    {
      initialize( codeName, DT_CONST_STRING, sizeof(bits_t), FlagShallow );
    }
    
    void const *ConstStringImpl::getDefaultData() const
    {
      static const bits_t defaultBits = { 0, 0 };
      return &defaultBits;
    }

    void ConstStringImpl::initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;

      while ( dst != dstEnd )
      {
        memcpy( dst, src, getAllocSize() );
        src += srcStride;
        dst += dstStride;
      }
    }

    void ConstStringImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;

      while ( dst != dstEnd )
      {
        memcpy( dst, src, getAllocSize() );
        src += srcStride;
        dst += dstStride;
      }
    }
    
    bool ConstStringImpl::equalsData( void const *lhs, void const *rhs ) const
    {
      bits_t const *lhsBits = reinterpret_cast<bits_t const *>(lhs);
      bits_t const *rhsBits = reinterpret_cast<bits_t const *>(rhs);
      if ( lhsBits->length != rhsBits->length )
        return false;
      else
        return memcmp( lhsBits->data, rhsBits->data, lhsBits->length ) == 0;
    }
    
    void ConstStringImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      bits_t const *bits = static_cast<bits_t const *>( data );
      encoder.makeString( bits->data, bits->length );
    }
    
    void ConstStringImpl::decodeJSON( JSON::Entity const &entity, void *dst ) const
    {
      throw Exception( "cannot set constant string from a JSON value" );
    }

    void ConstStringImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
    }
    
    bool ConstStringImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      return isConstString( impl->getType() );
    }

    std::string ConstStringImpl::descData( void const *data ) const
    {
      bits_t const *bits = static_cast<bits_t const *>( data );
      std::string result = "\"";
      if ( bits->length > 64 )
      {
        result.append( bits->data, 64 );
        result += "...";
      }
      else result.append( bits->data, bits->length );
      result += "\"";
      return result;
    }

    std::string ConstStringImpl::toString( void const *data ) const
    {
      bits_t const *bits = static_cast<bits_t const *>( data );
      return std::string( bits->data, bits->length );
    }
  }
}
