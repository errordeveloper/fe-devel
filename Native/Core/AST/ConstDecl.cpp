/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include "ConstDecl.h"
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/ByteAdapter.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/ExprValue.h>
#include <Fabric/Core/CG/IntegerAdapter.h>
#include <Fabric/Core/CG/Location.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/FloatAdapter.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/Util/Parse.h>
#include <Fabric/Base/Util/SimpleString.h>

#include <llvm/Constant.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( ConstDecl );

    RC::ConstHandle<ConstDecl> ConstDecl::Create(
      CG::Location const &location,
      std::string const &name,
      std::string const &type,
      std::string const &value
      )
    {
      return new ConstDecl( location, name, type, value );
    }
    
    ConstDecl::ConstDecl(
      CG::Location const &location,
      std::string const &name,
      std::string const &type,
      std::string const &value
      )
      : Node( location )
      , m_name( name )
      , m_type( type )
      , m_value( value )
    {
    }
    
    void ConstDecl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Node::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "name" ).makeString( m_name );
      jsonObjectEncoder.makeMember( "type" ).makeString( m_type );
      jsonObjectEncoder.makeMember( "value" ).makeString( m_value );
    }
    
    void ConstDecl::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      try
      {
        cgManager->getAdapter( m_type );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
    }

    void ConstDecl::llvmCompileToScope( CG::Scope &scope, CG::ModuleBuilder &moduleBuilder ) const
    {
      RC::Handle<CG::Manager> manager = moduleBuilder.getManager();
      
      RC::ConstHandle<CG::Adapter> adapter = manager->getAdapter( m_type );
      
      RC::ConstHandle<CG::ByteAdapter> byteAdapter = manager->getByteAdapter();
      RC::ConstHandle<CG::IntegerAdapter> integerAdapter = manager->getIntegerAdapter();
      RC::ConstHandle<CG::SizeAdapter> sizeAdapter = manager->getSizeAdapter();
      RC::ConstHandle<CG::FloatAdapter> scalarAdapter = manager->getFP32Adapter();
      
      CG::ExprValue exprValue( moduleBuilder.getContext() );
      if ( adapter == byteAdapter )
      {
        uint64_t uint64Value = Util::parseUInt64( m_value );
        if ( uint64Value > UINT8_MAX )
          throw CG::Error( getLocation(), "byte constant out-of-range" );
        uint8_t value = uint8_t(uint64Value);
        exprValue = CG::ExprValue( byteAdapter, CG::USAGE_RVALUE, moduleBuilder.getContext(), byteAdapter->llvmConst( moduleBuilder.getContext(), value ) );
      }
      else if ( adapter == integerAdapter )
      {
        uint64_t uint64Value = Util::parseUInt64( m_value );
        if ( uint64Value > INT32_MAX )
          throw CG::Error( getLocation(), "integer constant out-of-range" );
        int32_t value = int32_t(uint64Value);
        exprValue = CG::ExprValue( integerAdapter, CG::USAGE_RVALUE, moduleBuilder.getContext(), integerAdapter->llvmConst( moduleBuilder.getContext(), value ) );
      }
      else if ( adapter == sizeAdapter )
      {
        uint64_t uint64Value = Util::parseUInt64( m_value );
        if ( uint64Value > SIZE_MAX )
          throw CG::Error( getLocation(), "size constant out-of-range" );
        size_t value = size_t(uint64Value);
        exprValue = CG::ExprValue( sizeAdapter, CG::USAGE_RVALUE, moduleBuilder.getContext(), sizeAdapter->llvmConst( moduleBuilder.getContext(), value ) );
      }
      else if ( adapter == scalarAdapter )
        exprValue = CG::ExprValue( scalarAdapter, CG::USAGE_RVALUE, moduleBuilder.getContext(), scalarAdapter->llvmConst( moduleBuilder.getContext(), Util::parseDouble( m_value ) ) );
      else throw CG::Error( getLocation(), "constant declaration type must be Byte, Integer, Size or Scalar" );
        
      if ( scope.has( m_name ) )
        throw CG::Error( getLocation(), "symbol " + _(m_name) + " already exists" );
      else scope.put( m_name, CG::VariableSymbol::Create( exprValue ) );
    }
  };
};
