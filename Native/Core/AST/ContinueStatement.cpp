/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ContinueStatement.h"
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( ContinueStatement );
    
    RC::ConstHandle<ContinueStatement> ContinueStatement::Create( CG::Location const &location )
    {
      return new ContinueStatement( location );
    }
    
    ContinueStatement::ContinueStatement( CG::Location const &location )
      : Statement( location )
    {
    }
    
    void ContinueStatement::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Statement::appendJSONMembers( jsonObjectEncoder, includeLocation );
    }
    
    void ContinueStatement::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
    }

    void ContinueStatement::llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      CG::Scope &scope = basicBlockBuilder.getScope();
      scope.llvmContinue( basicBlockBuilder, getLocation() );
    }
  };
};
