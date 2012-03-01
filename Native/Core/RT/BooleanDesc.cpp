/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "BooleanDesc.h"

#include <Fabric/Core/RT/BooleanImpl.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    BooleanDesc::BooleanDesc(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<BooleanImpl> const &booleanImpl
      )
      : ComparableDesc(
        userNameBase,
        userNameArraySuffix,
        booleanImpl
        )
      , m_booleanImpl( booleanImpl )
    {
    }

    bool BooleanDesc::getValue( void const *data ) const
    {
      return m_booleanImpl->getValue( data );
    }
    
    void BooleanDesc::setValue( bool value, void *data ) const
    {
      m_booleanImpl->setValue( value, data );
    }
    
    void BooleanDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      ComparableDesc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "internalType" ).makeString( "boolean" );
    }
  }
}
