/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/ValueProducerImpl.h>
#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace RT
  {
    ValueProducerImpl::ValueProducerImpl( std::string const &codeName, RC::ConstHandle<RT::Impl> const &valueImpl )
      : m_valueImpl( valueImpl )
    {
      initialize( codeName, DT_VALUE_PRODUCER, sizeof(MR::ValueProducer const *) );
    }
    
    void const *ValueProducerImpl::getDefaultData() const
    {
      static MR::ValueProducer const *defaultData = 0;
      return &defaultData;
    }
    
    void ValueProducerImpl::initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;
      while ( dst != dstEnd )
      {
        MR::ValueProducer const *&dstBits = *reinterpret_cast<MR::ValueProducer const **>( dst );
        if ( src )
        {
          MR::ValueProducer const *srcBits = *reinterpret_cast<MR::ValueProducer const * const *>( src );
          dstBits = srcBits;
          if ( dstBits )
            dstBits->retain();
          src += srcStride;
        }
        else dstBits = 0;
        dst += dstStride;
      }
    }
    
    void ValueProducerImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;

      while ( dst != dstEnd )
      {
        MR::ValueProducer const *srcBits = *reinterpret_cast<MR::ValueProducer const * const *>( src );
        MR::ValueProducer const *&dstBits = *reinterpret_cast<MR::ValueProducer const **>( dst );
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
     
    bool ValueProducerImpl::equalsData( void const *lhs, void const *rhs ) const
    {
      MR::ValueProducer const *lhsBits = *static_cast<MR::ValueProducer const * const *>( lhs );
      MR::ValueProducer const *rhsBits = *static_cast<MR::ValueProducer const * const *>( rhs );
      return lhsBits == rhsBits;
    }
    
    void ValueProducerImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      throw Exception( "unable to convert ValueProducer to JSON" );
    }
    
    void ValueProducerImpl::decodeJSON( JSON::Entity const &entity, void *dst ) const
    {
      throw Exception( "unable to convert ValueProducer from JSON" );
    }

    void ValueProducerImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
      uint8_t * const dataEnd = data + count * stride;
      while ( data != dataEnd )
      {
        MR::ValueProducer const *bits = *reinterpret_cast<MR::ValueProducer const **>( data );
        if ( bits )
          bits->release();
        data += stride;
      }
    }
    
    std::string ValueProducerImpl::descData( void const *data ) const
    {
      return "ValueProducer";
    }
    
    bool ValueProducerImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      return isValueProducer( impl->getType() );
    }

    size_t ValueProducerImpl::getIndirectMemoryUsage( void const *data ) const
    {
      return 0;
    }
    
    RC::ConstHandle<Impl> ValueProducerImpl::getValueImpl() const
    {
      return m_valueImpl;
    }
  }
}
