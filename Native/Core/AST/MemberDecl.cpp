/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "MemberDecl.h"
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/Location.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( MemberDecl );
    
    MemberDecl::MemberDecl( CG::Location const &location, std::string const &name, std::string const &type )
      : Node( location )
      , m_name( name )
      , m_type( type )
    {
    }
    
    void MemberDecl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Node::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "name" ).makeString( m_name );
      jsonObjectEncoder.makeMember( "type" ).makeString( m_type );
    }
    
    void MemberDecl::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      try
      {
        cgManager->getAdapter( m_type );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
    }

    void MemberDecl::buildStructMemberInfo( RC::ConstHandle<RT::Manager> const &rtManager, RT::StructMemberInfo &structMemberInfo ) const
    {
      structMemberInfo.name = m_name;
      structMemberInfo.desc = rtManager->maybeGetDesc( m_type );
      if ( !structMemberInfo.desc )
        throw CG::Error( getLocation(), "member " + _(m_name) + ": type " + _(m_type) + " not registered" );
    }
  };
};
