/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "StructDesc.h"
#include "StructImpl.h"
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    StructDesc::StructDesc( std::string const &name, RC::ConstHandle<StructImpl> const &structImpl )
      : Desc( name, structImpl )
      , m_structImpl( structImpl )
    {
    }

    size_t StructDesc::getNumMembers() const
    {
      return m_structImpl->getNumMembers();
    }
    
    StructMemberInfo const &StructDesc::getMemberInfo( size_t index ) const
    {
      return m_structImpl->getMemberInfo( index );
    }
    
    void const *StructDesc::getMemberData( void const *data, size_t index ) const
    {
      return m_structImpl->getMemberData( data, index );
    }
    
    void *StructDesc::getMemberData( void *data, size_t index ) const
    {
      return m_structImpl->getMemberData( data, index );
    }
   
    bool StructDesc::hasMember( std::string const &name ) const
    {
      return m_structImpl->hasMember( name );
    }
    
    size_t StructDesc::getMemberIndex( std::string const &name ) const
    {
      return m_structImpl->getMemberIndex( name );
    }

    RC::Handle<RC::Object> StructDesc::getPrototype() const
    {
      return m_prototype;
    }
    
    void StructDesc::setPrototype( RC::Handle<RC::Object> const &prototype ) const
    {
      m_prototype = prototype;
    }
    
    void StructDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Desc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "internalType" ).makeString( "struct" );
      JSON::Encoder membersEncoder = resultObjectEncoder.makeMember( "members" );
      JSON::ArrayEncoder membersArrayEncoder = membersEncoder.makeArray();
      size_t numMembers = getNumMembers();
      for ( size_t i=0; i<numMembers; ++i )
      {
        RT::StructMemberInfo const &memberInfo = getMemberInfo( i );
        JSON::Encoder memberEncoder = membersArrayEncoder.makeElement();
        JSON::ObjectEncoder memberObjectEncoder = memberEncoder.makeObject();
        memberObjectEncoder.makeMember( "name" ).makeString( memberInfo.name );
        memberObjectEncoder.makeMember( "type" ).makeString( memberInfo.desc->getUserName() );
      }
    }
  };
};
