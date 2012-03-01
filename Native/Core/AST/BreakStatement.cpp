/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/BreakStatement.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( BreakStatement );
    
    RC::ConstHandle<BreakStatement> BreakStatement::Create( CG::Location const &location )
    {
      return new BreakStatement( location );
    }

    BreakStatement::BreakStatement( CG::Location const &location )
      : Statement( location )
    {
    }
    
    void BreakStatement::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Statement::appendJSONMembers( jsonObjectEncoder, includeLocation );
    }
    
    void BreakStatement::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
    }

    void BreakStatement::llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      CG::Scope &scope = basicBlockBuilder.getScope();
      scope.llvmBreak( basicBlockBuilder, getLocation() );
    }
  };
};
