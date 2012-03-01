/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/BooleanImpl.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace RT
  {
    BooleanImpl::BooleanImpl( std::string const &codeName )
      : SimpleImpl( codeName, DT_BOOLEAN, sizeof(bool) )
    {
    }
    
    void BooleanImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      encoder.makeBoolean( getValue(data) );
    }
    
    void BooleanImpl::setData( void const *src, void *dst ) const
    {
      setValue( getValue( src ), dst );
    }
    
    void const *BooleanImpl::getDefaultData() const
    {
      static bool const defaultData = 0;
      return &defaultData;
    }
    
    void BooleanImpl::decodeJSON( JSON::Entity const &entity, void *dst ) const
    {
      entity.requireBoolean();
      setValue( entity.booleanValue(), dst );
    }
    
    std::string BooleanImpl::descData( void const *data ) const
    {
      return _( getValue(data) );
    }

    size_t BooleanImpl::hash( void const *data ) const
    {
      return size_t( getValue( data ) );
    }
    
    int BooleanImpl::compare( void const *lhsData, void const *rhsData ) const
    {
      return int( getValue( lhsData ) ) - int( getValue( rhsData ) );
    }
  };
};
