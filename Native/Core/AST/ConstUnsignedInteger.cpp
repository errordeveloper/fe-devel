/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/AST/ConstUnsignedInteger.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/IntegerAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/Util/Parse.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( ConstUnsignedInteger );
    
    RC::ConstHandle<ConstUnsignedInteger> ConstUnsignedInteger::Create( CG::Location const &location, std::string const &valueString )
    {
      return new ConstUnsignedInteger( location, valueString );
    }
    
    ConstUnsignedInteger::ConstUnsignedInteger( CG::Location const &location, std::string const &valueString )
      : Expr( location )
      , m_valueString( valueString )
    {
    }
    
    void ConstUnsignedInteger::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember("valueString").makeString( m_valueString );
    }
    
    void ConstUnsignedInteger::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      cgManager->getIntegerAdapter();
    }
    
    CG::ExprType ConstUnsignedInteger::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      uint64_t uint64Value = Util::parseUInt64( m_valueString );
      RC::ConstHandle<CG::Adapter> adapter;
      if ( uint64Value <= INT32_MAX )
        adapter = basicBlockBuilder.getManager()->getIntegerAdapter();
      else if ( uint64Value <= SIZE_MAX )
        adapter = basicBlockBuilder.getManager()->getSizeAdapter();
      else throw CG::Error( getLocation(), "integer contant too large" );
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      return CG::ExprType( adapter, CG::USAGE_RVALUE );
    }
    
    CG::ExprValue ConstUnsignedInteger::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      if ( usage == CG::USAGE_LVALUE )
        throw Exception( "constants cannot be used as l-values" );
      uint64_t uint64Value = Util::parseUInt64( m_valueString );
      if ( uint64Value <= INT32_MAX )
      {
        RC::ConstHandle<CG::IntegerAdapter> integerAdapter = basicBlockBuilder.getManager()->getIntegerAdapter();
        integerAdapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
        return CG::ExprValue( integerAdapter, CG::USAGE_RVALUE, basicBlockBuilder.getContext(), integerAdapter->llvmConst( basicBlockBuilder.getContext(), int32_t(uint64Value) ) );
      }
      else if ( uint64Value <= SIZE_MAX )
      {
        RC::ConstHandle<CG::SizeAdapter> sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
        sizeAdapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
        return CG::ExprValue( sizeAdapter, CG::USAGE_RVALUE, basicBlockBuilder.getContext(), sizeAdapter->llvmConst( basicBlockBuilder.getContext(), size_t(uint64Value) ) );
      }
      else throw CG::Error( getLocation(), "integer contant too large" );
    }
  }
}
