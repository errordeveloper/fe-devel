/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "VariableArrayDesc.h"
#include "VariableArrayImpl.h"

#include <Fabric/Base/JSON/String.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Core/Util/JSONGenerator.h>

namespace Fabric
{
  namespace RT
  {
    VariableArrayDesc::VariableArrayDesc( std::string const &name, RC::ConstHandle<VariableArrayImpl> const &variableArrayImpl, RC::ConstHandle<Desc> const &memberDesc )
      : ArrayDesc( name, variableArrayImpl, memberDesc )
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
    
    void VariableArrayDesc::jsonDesc( Util::JSONObjectGenerator &resultJOG ) const
    {
      ArrayDesc::jsonDesc( resultJOG );
      resultJOG.makeMember( "internalType" ).makeString( "variableArray" );
      resultJOG.makeMember( "memberType" ).makeString( getMemberDesc()->getUserName() );
    }
  };
};
