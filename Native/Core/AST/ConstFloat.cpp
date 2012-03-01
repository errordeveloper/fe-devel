/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ConstFloat.h"
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/FloatAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/Util/Parse.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( ConstFloat );
    
    RC::ConstHandle<ConstFloat> ConstFloat::Create( CG::Location const &location, std::string const &valueString )
    {
      return new ConstFloat( location, valueString );
    }
    
    ConstFloat::ConstFloat( CG::Location const &location, std::string const &valueString )
      : Expr( location )
      , m_valueString( valueString )
    {
    }
    
    void ConstFloat::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "value" ).makeString( m_valueString );
    }
    
    void ConstFloat::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      cgManager->getFloat64Adapter();
    }
    
    CG::ExprType ConstFloat::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = basicBlockBuilder.getManager()->getFloat64Adapter();
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return CG::ExprType( adapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue ConstFloat::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "constants cannot be used as l-values" );
      RC::ConstHandle<CG::FloatAdapter> floatAdapter = basicBlockBuilder.getManager()->getFloat64Adapter();
      floatAdapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      double value = Util::parseDouble( m_valueString );
      return CG::ExprValue( floatAdapter, CG::USAGE_RVALUE, basicBlockBuilder.getContext(), floatAdapter->llvmConst( basicBlockBuilder.getContext(), value ) );
    }
  }
}
