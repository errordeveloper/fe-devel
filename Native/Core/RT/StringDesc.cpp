/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "StringDesc.h"
#include "StringImpl.h"
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    StringDesc::StringDesc(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<StringImpl> const &stringImpl
      )
      : ComparableDesc(
        userNameBase,
        userNameArraySuffix,
        stringImpl
        )
      , m_stringImpl( stringImpl )
    {
    }

    char const *StringDesc::getValueData( void const *src ) const
    {
      return m_stringImpl->getValueData( src );
    }
    
    size_t StringDesc::getValueLength( void const *src ) const
    {
      return m_stringImpl->getValueLength( src );
    }
    
    void StringDesc::setValue( char const *cStr, size_t length, void *dst ) const
    {
      return StringImpl::SetValue( cStr, length, dst );
    }
    
    void StringDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Desc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "internalType" ).makeString( "string" );
    }
  }
}
