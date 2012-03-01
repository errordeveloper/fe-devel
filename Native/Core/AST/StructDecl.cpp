/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "StructDecl.h"
#include "MemberDeclVector.h"
#include <Fabric/Core/CG/Diagnostics.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/StructDesc.h>
#include <Fabric/Core/RT/StructMemberInfo.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( StructDecl );
    
    RC::ConstHandle<StructDecl> StructDecl::Create(
      CG::Location const &location,
      std::string const &name,
      RC::ConstHandle<MemberDeclVector> const &members
      )
    {
      return new StructDecl( location, name, members );
    }
    
    StructDecl::StructDecl(
      CG::Location const &location,
      std::string const &name,
      RC::ConstHandle<MemberDeclVector> const &members
      )
      : Global( location )
      , m_name( name )
      , m_members( members )
    {
    }
    
    void StructDecl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Global::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "name" ).makeString( m_name );
      m_members->appendJSON( jsonObjectEncoder.makeMember( "members" ), includeLocation );
    }
    
    void StructDecl::registerTypes( RC::Handle<RT::Manager> const &rtManager, CG::Diagnostics &diagnostics ) const
    {
      RT::StructMemberInfoVector structMemberInfoVector;
      m_members->buildStructMemberInfoVector( rtManager, structMemberInfoVector );
      try
      {
        rtManager->registerStruct( m_name, structMemberInfoVector );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
    }
    
    void StructDecl::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_members->registerTypes( cgManager, diagnostics );
      registerTypes( cgManager->getRTManager(), diagnostics );
    }

    void StructDecl::llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const
    {
    }
  };
};
