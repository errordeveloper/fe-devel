/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Destructor.h"
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( Destructor );
    
    RC::ConstHandle<Destructor> Destructor::Create(
      CG::Location const &location,
      std::string const &thisTypeName,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body
      )
    {
      return new Destructor( location, thisTypeName, symbolName, body );
    }
    
    Destructor::Destructor(
      CG::Location const &location,
      std::string const &thisTypeName,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body
      )
      : FunctionBase(
        location,
        "",
        symbolName,
        body,
        false
        )
      , m_thisTypeName( thisTypeName )
    {
    }
    
    void Destructor::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      FunctionBase::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "thisTypeName" ).makeString( m_thisTypeName );
    }
    
    std::string Destructor::getThisTypeName() const
    {
      return m_thisTypeName;
    }
    
    std::string Destructor::getPencilKey( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return CG::DestructorPencilKey( cgManager->getAdapter( m_thisTypeName ) );
    }
    
    std::string Destructor::getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return CG::DestructorDefaultSymbolName( cgManager->getAdapter( m_thisTypeName ) );
    }
    
    std::string Destructor::getDesc( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return CG::DestructorDesc( cgManager->getAdapter( m_thisTypeName ) );
    }

    RC::ConstHandle<ParamVector> Destructor::getParams( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return ParamVector::Create( Param::Create( getLocation(), "this", m_thisTypeName, CG::USAGE_LVALUE ) );
    }

    void Destructor::llvmCompileToModule(
      CG::ModuleBuilder &moduleBuilder,
      CG::Diagnostics &diagnostics,
      bool buildFunctionBodies
      ) const
    {
      FunctionBase::llvmCompileToModule( moduleBuilder, diagnostics, buildFunctionBodies );
      moduleBuilder.setDestructorName( m_thisTypeName, getSymbolName( moduleBuilder.getManager() ) );
    }
  }
}
