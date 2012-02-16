/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
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
      : Impl( codeName, DT_OPAQUE )
    {
      setSize( size );
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
    
    void OpaqueImpl::setData( void const *src, void *dst ) const
    {
      memcpy( dst, src, getAllocSize() );
    }
   
    void OpaqueImpl::disposeDatasImpl( void *data, size_t count, size_t stride ) const
    {
    }
    
    void OpaqueImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      encoder.makeNull();
    }
    
    void OpaqueImpl::decodeJSON( JSON::Entity const &entity, void *dst ) const
    {
      entity.requireNull();
      memset( dst, 0, getAllocSize() );
    }
    
    std::string OpaqueImpl::descData( void const *src ) const
    {
      return "Opaque<" + Util::hexBuf( getAllocSize(), src ) + ">";
    }

    bool OpaqueImpl::isShallow() const
    {
      return true;
    }

    bool OpaqueImpl::isNoAliasSafe() const
    {
      return true;
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
    
    bool OpaqueImpl::isExportable() const
    {
      return false;
    }
  }
}
