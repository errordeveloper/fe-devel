/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ConstDeclStatement.h"
#include "ConstDecl.h"
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( ConstDeclStatement );
    
    RC::ConstHandle<ConstDeclStatement> ConstDeclStatement::Create(
      CG::Location const &location,
      RC::ConstHandle<ConstDecl> const &constDecl
      )
    {
      return new ConstDeclStatement( location, constDecl );
    }

    ConstDeclStatement::ConstDeclStatement(
      CG::Location const &location,
      RC::ConstHandle<ConstDecl> const &constDecl
      )
      : Statement( location )
      , m_constDecl( constDecl )
    {
    }
    
    void ConstDeclStatement::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Node::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_constDecl->appendJSON( jsonObjectEncoder.makeMember( "constDecl" ), includeLocation );
    }
    
    void ConstDeclStatement::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_constDecl->registerTypes( cgManager, diagnostics );
    }
    
    void ConstDeclStatement::llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      try
      {
        m_constDecl->llvmCompileToScope( basicBlockBuilder.getScope(), basicBlockBuilder.getModuleBuilder() );
      }
      catch ( CG::Error e )
      {
        addError( diagnostics, e );
      }
    }
  };
};
