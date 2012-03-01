/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "GlobalConstDecl.h"
#include "ConstDecl.h"
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( GlobalConstDecl );
    
    GlobalConstDecl::GlobalConstDecl(
      CG::Location const &location,
      RC::ConstHandle<ConstDecl> const &constDecl
      )
      : Global( location )
      , m_constDecl( constDecl )
    {
    }
    
    void GlobalConstDecl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Global::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_constDecl->appendJSON( jsonObjectEncoder.makeMember( "constDecl" ), includeLocation );
    }
    
    void GlobalConstDecl::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_constDecl->registerTypes( cgManager, diagnostics );
    }
    
    void GlobalConstDecl::llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const
    {
      if ( !buildFunctionBodies )
      {
        try
        {
          m_constDecl->llvmCompileToScope( moduleBuilder.getScope(), moduleBuilder );
        }
        catch ( CG::Error e )
        {
          addError( diagnostics, e );
        }
      }
    }
  };
};
