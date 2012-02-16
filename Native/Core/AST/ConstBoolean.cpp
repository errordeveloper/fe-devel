/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#include "ConstBoolean.h"
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/BooleanAdapter.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Base/Util/SimpleString.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( ConstBoolean );
    
    RC::ConstHandle<ConstBoolean> ConstBoolean::Create( CG::Location const &location, bool value )
    {
      return new ConstBoolean( location, value );
    }
    
    ConstBoolean::ConstBoolean( CG::Location const &location, bool value )
      : Expr( location )
      , m_value( value )
    {
    }
    
    void ConstBoolean::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "value" ).makeBoolean( m_value );
    }
    
    void ConstBoolean::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
    }
    
    CG::ExprType ConstBoolean::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = basicBlockBuilder.getManager()->getBooleanAdapter();
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return CG::ExprType( adapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue ConstBoolean::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "constants cannot be used as l-values" );
      RC::ConstHandle<CG::BooleanAdapter> booleanAdapter = basicBlockBuilder.getManager()->getBooleanAdapter();
      return CG::ExprValue( booleanAdapter, CG::USAGE_RVALUE, basicBlockBuilder.getContext(), booleanAdapter->llvmConst( basicBlockBuilder.getContext(), m_value ) );
    }
  };
};
