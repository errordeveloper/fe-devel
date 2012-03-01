/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "CompoundStatement.h"
#include <Fabric/Core/AST/StatementVector.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( CompoundStatement );
    
    RC::ConstHandle<CompoundStatement> CompoundStatement::Create(
      CG::Location const &location,
      RC::ConstHandle<StatementVector> const &statements
      )
    {
      return new CompoundStatement( location, statements );
    }
    
    CompoundStatement::CompoundStatement(
      CG::Location const &location,
      RC::ConstHandle<StatementVector> const &statements
      )
      : Statement( location )
      , m_statements( statements )
    {
    }
    
    void CompoundStatement::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Statement::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_statements->appendJSON( jsonObjectEncoder.makeMember( "statements" ), includeLocation );
    }
    
    void CompoundStatement::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_statements->registerTypes( cgManager, diagnostics );
    }

    void CompoundStatement::llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      CG::Scope subScope( basicBlockBuilder.getScope() );
      {
        CG::BasicBlockBuilder subBasicBlockBuilder( basicBlockBuilder, subScope );
        m_statements->llvmCompileToBuilder( subBasicBlockBuilder, diagnostics );
      }
      if ( !basicBlockBuilder->GetInsertBlock()->getTerminator() )
        subScope.llvmUnwind( basicBlockBuilder );
    }
  };
};
