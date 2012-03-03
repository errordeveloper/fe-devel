/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "OpaqueDesc.h"
#include <Fabric/Core/RT/OpaqueImpl.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    OpaqueDesc::OpaqueDesc(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<OpaqueImpl> const &opaqueImpl
      )
      : Desc(
        userNameBase,
        userNameArraySuffix,
        opaqueImpl
        )
      , m_opaqueImpl( opaqueImpl )
    {
    }
    
    void OpaqueDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Desc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "internalType" ).makeString( "opaque" );
    }
  };
};
