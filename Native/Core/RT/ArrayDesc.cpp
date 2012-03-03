/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ArrayDesc.h"

#include <Fabric/Core/RT/ArrayImpl.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    ArrayDesc::ArrayDesc(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<ArrayImpl> const &arrayImpl,
      RC::ConstHandle<Desc> const &memberDesc
      )
      : Desc(
        userNameBase,
        userNameArraySuffix,
        arrayImpl
        )
      , m_arrayImpl( arrayImpl )
      , m_memberDesc( memberDesc )
    {
    }
    
    RC::ConstHandle<ArrayImpl> ArrayDesc::getImpl() const
    {
      return m_arrayImpl;
    }
    
    RC::ConstHandle<Desc> ArrayDesc::getMemberDesc() const
    {
      return m_memberDesc;
    }

    size_t ArrayDesc::getNumMembers( void const *data ) const
    {
      return m_arrayImpl->getNumMembers( data );
    }
    
    void const *ArrayDesc::getImmutableMemberData( void const *data, size_t index ) const
    {
      return m_arrayImpl->getImmutableMemberData( data, index );
    }
    
    void *ArrayDesc::getMutableMemberData( void *data, size_t index ) const
    {
      return m_arrayImpl->getMutableMemberData( data, index );
    }
    
    void ArrayDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Desc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "memberType" ).makeString( getMemberDesc()->getUserName() );
    }
  }
}
