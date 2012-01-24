/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/RT/ConstStringDesc.h>
#include <Fabric/Core/RT/ConstStringImpl.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    ConstStringDesc::ConstStringDesc( std::string const &name, RC::ConstHandle<ConstStringImpl> const &constStringImpl )
      : Desc( name, constStringImpl )
      , m_constStringImpl( constStringImpl )
    {
    }
    
    void ConstStringDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Desc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "internalType" ).makeString( "ConstString" );
    }

    std::string ConstStringDesc::toString( void const *data ) const
    {
      return m_constStringImpl->toString( data );
    }

    char const *ConstStringDesc::getValueData( void const *src ) const
    {
      return m_constStringImpl->getValueData( src );
    }
    
    size_t ConstStringDesc::getValueLength( void const *src ) const
    {
      return m_constStringImpl->getValueLength( src );
    }
  };
};
