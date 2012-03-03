/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/SlicedArrayDesc.h>
#include <Fabric/Core/RT/SlicedArrayImpl.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    SlicedArrayDesc::SlicedArrayDesc(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<SlicedArrayImpl> const &slicedArrayImpl,
      RC::ConstHandle<Desc> const &memberDesc
      )
      : ArrayDesc(
        userNameBase,
        userNameArraySuffix,
        slicedArrayImpl,
        memberDesc
        )
      , m_slicedArrayImpl( slicedArrayImpl )
    {
    }

    RC::ConstHandle<RT::SlicedArrayImpl> SlicedArrayDesc::getImpl() const
    {
      return RC::ConstHandle<RT::SlicedArrayImpl>::StaticCast( Desc::getImpl() );
    }
    
    void SlicedArrayDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      ArrayDesc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "internalType" ).makeString( "slicedArray" );
      resultObjectEncoder.makeMember( "memberType" ).makeString( getMemberDesc()->getUserName() );
    }

    void SlicedArrayDesc::setNumMembers( void *data, size_t numMembers, void const *defaultMemberData ) const
    {
      return m_slicedArrayImpl->setNumMembers( data, numMembers, defaultMemberData );
    }
  }
}
