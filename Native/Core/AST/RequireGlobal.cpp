/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/RequireGlobal.h>
#include <Fabric/Core/AST/RequireVector.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( RequireGlobal );
    
    RC::ConstHandle<RequireGlobal> RequireGlobal::Create(
      CG::Location const &location,
      RC::ConstHandle<RequireVector> const &useVector
      )
    {
      return new RequireGlobal( location, useVector );
    }

    RequireGlobal::RequireGlobal(
      CG::Location const &location,
      RC::ConstHandle<RequireVector> const &useVector
      )
      : Global( location )
      , m_requireVector( useVector )
    {
    }
    
    void RequireGlobal::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Global::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_requireVector->appendJSON( jsonObjectEncoder.makeMember( "requireVector" ), includeLocation );
    }
    
    void RequireGlobal::collectRequires( RequireNameToLocationMap &uses ) const
    {
      m_requireVector->collectRequires( uses );
    }
    
    void RequireGlobal::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
    }
    
    void RequireGlobal::llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const
    {
    }
  }
}
