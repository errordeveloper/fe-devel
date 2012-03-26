/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/OpaqueImpl.h>

#include <Fabric/Core/Util/Hex.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace RT
  {
    OpaqueImpl::OpaqueImpl( std::string const &codeName, size_t size )
    {
      initialize( codeName, DT_OPAQUE, size, FlagShallow | FlagExportable );

      m_defaultData = malloc( size );
      memset( m_defaultData, 0, size );
    }
    
    OpaqueImpl::~OpaqueImpl()
    {
      free( m_defaultData );
    }
    
    void const *OpaqueImpl::getDefaultData() const
    {
      return m_defaultData;
    }
    
    void OpaqueImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      size_t allocSize = getAllocSize();
      
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;

      while ( dst != dstEnd )
      {
        memcpy( dst, src, allocSize );
        src += srcStride;
        dst += dstStride;
      }
    }
   
    void OpaqueImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
    }
    
    void OpaqueImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      encoder.makeBoolean( memcmp( data, m_defaultData, getAllocSize() ) != 0 );
    }
    
    void OpaqueImpl::decodeJSON( JSON::Entity const &entity, void *dst ) const
    {
      entity.requireNullOrBoolean();
    }
    
    std::string OpaqueImpl::descData( void const *src ) const
    {
      return "Opaque<" + Util::hexBuf( getAllocSize(), src ) + ">";
    }

    bool OpaqueImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      if ( !isOpaque( impl->getType() ) )
        return false;
      return getAllocSize() == impl->getAllocSize();
    }

    bool OpaqueImpl::equalsData( void const *lhs, void const *rhs ) const
    {
      return memcmp( lhs, rhs, getAllocSize() ) == 0;
    }
  }
}
