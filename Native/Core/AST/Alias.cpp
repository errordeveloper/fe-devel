/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/Alias.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( Alias );
    
    RC::ConstHandle<Alias> Alias::Create(
      CG::Location const &location,
      std::string const &name,
      std::string const &adapterName
      )
    {
      return new Alias( location, name, adapterName );
    }

    Alias::Alias(
      CG::Location const &location,
      std::string const &name,
      std::string const &adapterName
      )
      : Global( location )
      , m_name( name )
      , m_adapterName( adapterName )
    {
    }
    
    void Alias::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Global::appendJSONMembers( jsonObjectEncoder, includeLocation );
      {
        JSON::Encoder memberEncoder = jsonObjectEncoder.makeMember( "newTypeName" );
        memberEncoder.makeString( m_name );
      }
      {
        JSON::Encoder memberEncoder = jsonObjectEncoder.makeMember( "oldTypeName" );
        memberEncoder.makeString( m_adapterName );
      }
    }
    
    void Alias::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      try
      {
        RC::Handle<RT::Manager> rtManager = cgManager->getRTManager();
        RC::ConstHandle<RT::Desc> oldDesc = rtManager->getDesc( m_adapterName );
        RC::ConstHandle<RT::Desc> newDesc = rtManager->registerAlias( m_name, oldDesc );
        cgManager->getAdapter( newDesc );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
    }
    
    void Alias::llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const
    {
      try
      {
        RC::ConstHandle<CG::Adapter> aliasAdapter = moduleBuilder.getAdapter( m_name, getLocation() );
        aliasAdapter->llvmCompileToModule( moduleBuilder );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
    }
  };
};
