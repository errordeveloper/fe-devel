/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Node.h"
#include <Fabric/Core/CG/Diagnostics.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    Node::Node( CG::Location const &location )
      : m_location( location )
    {
    }
    
    void Node::appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const
    {
      appendJSONMembers( encoder.makeObject(), includeLocation );
    }
    
    void Node::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      jsonObjectEncoder.makeMember( "+" ).makeString( nodeTypeName() );
      if ( includeLocation )
        m_location.appendJSON( jsonObjectEncoder.makeMember( "-" ), includeLocation );
    }

    void Node::addWarning( CG::Diagnostics &diagnostics, Util::SimpleString const &desc ) const
    {
      diagnostics.addWarning( m_location, desc );
    }
    
    void Node::addError( CG::Diagnostics &diagnostics, Util::SimpleString const &desc ) const
    {
	 		diagnostics.addError( m_location, desc );
    }
    
    void Node::addError( CG::Diagnostics &diagnostics, CG::Error const &error ) const
    {
      diagnostics.addError( error.getLocation(), error.getDesc() );
    }
  };
};
