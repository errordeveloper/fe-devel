/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "VariableArrayDesc.h"
#include "VariableArrayImpl.h"

#include <Fabric/Base/Exception.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    VariableArrayDesc::VariableArrayDesc(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<VariableArrayImpl> const &variableArrayImpl,
      RC::ConstHandle<Desc> const &memberDesc
      )
      : ArrayDesc(
        userNameBase,
        userNameArraySuffix,
        variableArrayImpl,
        memberDesc
        )
      , m_variableArrayImpl( variableArrayImpl )
    {
    }
    
    void VariableArrayDesc::push( void *dst, void const *src ) const
    {
      m_variableArrayImpl->push( dst, src );
    }
    
    void VariableArrayDesc::pop( void *data, void *result ) const
    {
      m_variableArrayImpl->pop( data, result );
    }
    
    void VariableArrayDesc::append( void *dst, void const *src ) const
    {
      m_variableArrayImpl->append( dst, src );
    }

    void VariableArrayDesc::setMembers( void *data, size_t numMembers, void const *members ) const
    {
      m_variableArrayImpl->setMembers( data, numMembers, members );
    }
    
    void VariableArrayDesc::setNumMembers( void *data, size_t newNumMembers, void const *defaultMemberData ) const
    {
      m_variableArrayImpl->setNumMembers( data, newNumMembers, defaultMemberData );
    }

    RC::ConstHandle<RT::VariableArrayImpl> VariableArrayDesc::getImpl() const
    {
      return RC::ConstHandle<RT::VariableArrayImpl>::StaticCast( Desc::getImpl() );
    }
    
    void VariableArrayDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      ArrayDesc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "internalType" ).makeString( "variableArray" );
      resultObjectEncoder.makeMember( "memberType" ).makeString( getMemberDesc()->getUserName() );
    }
  }
}
