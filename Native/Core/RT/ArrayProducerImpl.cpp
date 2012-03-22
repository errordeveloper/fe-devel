/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/ArrayProducerImpl.h>
#include <Fabric/Core/MR/ArrayProducer.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace RT
  {
    ArrayProducerImpl::ArrayProducerImpl( std::string const &codeName, RC::ConstHandle<RT::Impl> const &elementImpl )
      : m_elementImpl( elementImpl )
    {
      initialize( codeName, DT_ARRAY_PRODUCER, sizeof(MR::ArrayProducer const *) );
    }
    
    void const *ArrayProducerImpl::getDefaultData() const
    {
      static MR::ArrayProducer const *defaultData = 0;
      return &defaultData;
    }
    
    void ArrayProducerImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      uint8_t *dstEnd = dst + dstStride * count;

      while ( dst != dstEnd )
      {
        MR::ArrayProducer const *srcBits = *reinterpret_cast<MR::ArrayProducer const * const *>( src );
        MR::ArrayProducer const *&dstBits = *reinterpret_cast<MR::ArrayProducer const **>( dst );
        if ( dstBits != srcBits )
        {
          if ( dstBits )
            dstBits->release();
          
          dstBits = srcBits;
          
          if ( dstBits )
            dstBits->retain();
        }
        src += srcStride;
        dst += dstStride;
      }
    }
     
    bool ArrayProducerImpl::equalsData( void const *lhs, void const *rhs ) const
    {
      MR::ArrayProducer const *lhsBits = *static_cast<MR::ArrayProducer const * const *>( lhs );
      MR::ArrayProducer const *rhsBits = *static_cast<MR::ArrayProducer const * const *>( rhs );
      return lhsBits == rhsBits;
    }
    
    void ArrayProducerImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      throw Exception( "unable to convert ArrayProducer to JSON" );
    }
    
    void ArrayProducerImpl::decodeJSON( JSON::Entity const &entity, void *dst ) const
    {
      throw Exception( "unable to convert ArrayProducer from JSON" );
    }

    void ArrayProducerImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
      uint8_t * const dataEnd = data + count * stride;
      while ( data != dataEnd )
      {
        MR::ArrayProducer const *bits = *reinterpret_cast<MR::ArrayProducer const **>( data );
        if ( bits )
          bits->release();
        data += stride;
      }
    }
    
    std::string ArrayProducerImpl::descData( void const *data ) const
    {
      return "ArrayProducer";
    }
    
    bool ArrayProducerImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      return isArrayProducer( impl->getType() );
    }

    size_t ArrayProducerImpl::getIndirectMemoryUsage( void const *data ) const
    {
      return 0;
    }
    
    RC::ConstHandle<Impl> ArrayProducerImpl::getElementImpl() const
    {
      return m_elementImpl;
    }
  }
}
