/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ConstString.h"
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/ConstStringAdapter.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/StringAdapter.h>
#include <Fabric/Core/Util/Parse.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( ConstString );
    
    RC::ConstHandle<ConstString> ConstString::Create( CG::Location const &location, std::string const &value )
    {
      return new ConstString( location, value );
    }

    ConstString::ConstString( CG::Location const &location, std::string const &value )
      : Expr( location )
      , m_value( value )
    {
    }
    
    void ConstString::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "value" ).makeString( m_value );
    }
    
    void ConstString::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      std::string unquotedValue;
      try
      {
        unquotedValue = Util::parseQuotedString( m_value );
      }
      catch ( Exception e )
      {
        throw CG::Error( getLocation(), e.getDesc() + "(" + m_value + ")" );
      }

      cgManager->getConstStringAdapter();
      cgManager->getStringAdapter();
    }
    
    CG::ExprType ConstString::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = basicBlockBuilder.getManager()->getStringAdapter();
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return CG::ExprType( adapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue ConstString::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "constants cannot be used as l-values" );
      std::string unquotedValue;
      try
      {
        unquotedValue = Util::parseQuotedString( m_value );
      }
      catch ( Exception e )
      {
        throw CG::Error( getLocation(), e.getDesc() + "(" + m_value + ")" );
      }
      RC::ConstHandle<CG::ConstStringAdapter> constStringAdapter = basicBlockBuilder.getManager()->getConstStringAdapter();
      constStringAdapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      RC::ConstHandle<CG::StringAdapter> stringAdapter = basicBlockBuilder.getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return CG::ExprValue( constStringAdapter, CG::USAGE_RVALUE, basicBlockBuilder.getContext(), constStringAdapter->llvmConst( basicBlockBuilder, unquotedValue.data(), unquotedValue.length() ) ).castTo( basicBlockBuilder, stringAdapter );
    }
  };
};
