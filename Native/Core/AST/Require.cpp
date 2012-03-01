/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/Require.h>
#include <Fabric/Core/AST/RequireInfo.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( Require );
    
    RC::ConstHandle<Require> Require::Create(
      CG::Location const &location,
      std::string const &name
      )
    {
      return new Require( location, name );
    }
    
    Require::Require(
      CG::Location const &location,
      std::string const &name
      )
      : Node( location )
      , m_name( name )
    {
    }
    
    void Require::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Node::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "name" ).makeString( m_name );
    }
    
    void Require::collectRequires( RequireNameToLocationMap &uses ) const
    {
      uses.insert( RequireNameToLocationMap::value_type( m_name, getLocation() ) );
    }
  }
}
