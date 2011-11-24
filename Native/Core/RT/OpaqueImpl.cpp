/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "OpaqueImpl.h"

#include <Fabric/Base/JSON/Null.h>
#include <Fabric/Core/Util/Encoder.h>
#include <Fabric/Core/Util/Decoder.h>
#include <Fabric/Core/Util/Hex.h>
#include <Fabric/Core/Util/JSONGenerator.h>
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
    
    RC::Handle<JSON::Value> OpaqueImpl::getJSONValue( void const *data ) const
    {
      return JSON::Null::Create();
    }
    
    void OpaqueImpl::generateJSON( void const *data, Util::JSONGenerator &jsonGenerator ) const
    {
      jsonGenerator.makeNull();
    }

    void OpaqueImpl::setDataFromJSONValue( RC::ConstHandle<JSON::Value> const &jsonValue, void *dst ) const
    {
      if ( !jsonValue->isNull() )
        throw Exception("value is not null");
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
    
    bool OpaqueImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      if ( !isOpaque( impl->getType() ) )
        return false;
      return getAllocSize() == impl->getAllocSize();
    }

    int OpaqueImpl::compareData( void const *lhs, void const *rhs ) const
    {
      return memcmp( lhs, rhs, getAllocSize() );
    }
  }; // namespace RT
}; // namespace FABRIC
