/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "StructDecl.h"
#include "MemberDeclVector.h"
#include <Fabric/Core/CG/Diagnostics.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/StructDesc.h>
#include <Fabric/Core/RT/StructMemberInfo.h>
#include <Fabric/Core/Util/SimpleString.h>

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
    
    void StructDecl::appendJSONMembers( Util::JSONObjectGenerator const &jsonObjectGenerator ) const
    {
      Global::appendJSONMembers( jsonObjectGenerator );
      jsonObjectGenerator.makeMember( "name" ).makeString( m_name );
      m_members->appendJSON( jsonObjectGenerator.makeMember( "members" ) );
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
        addError( diagnostics, e.getDesc() );
      }
    }
    
    void StructDecl::llvmPrepareModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics ) const
    {
      m_members->llvmPrepareModule( moduleBuilder, diagnostics );
      registerTypes( moduleBuilder.getManager()->getRTManager(), diagnostics );
    }

    void StructDecl::llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const
    {
    }
  };
};
