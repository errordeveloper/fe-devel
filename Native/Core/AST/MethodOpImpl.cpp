/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#include <Fabric/Core/AST/MethodOpImpl.h>
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/OverloadNames.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( MethodOpImpl );
    
    RC::ConstHandle<MethodOpImpl> MethodOpImpl::Create(
      CG::Location const &location,
      std::string const &returnTypeName,
      std::string const &thisTypeName,
      std::string const &methodName,
      RC::ConstHandle<ParamVector> const &params,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body
      )
    {
      return new MethodOpImpl( location, returnTypeName, thisTypeName, methodName, params, symbolName, body );
    }
    
    MethodOpImpl::MethodOpImpl(
      CG::Location const &location,
      std::string const &returnTypeName,
      std::string const &thisTypeName,
      std::string const &methodName,
      RC::ConstHandle<ParamVector> const &params,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body
      )
      : FunctionBase(
        location,
        returnTypeName,
        symbolName,
        body,
        false
        )
      , m_selfTypeName( thisTypeName )
      , m_methodName( methodName )
      , m_params( params )
    {
    }
    
    void MethodOpImpl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      FunctionBase::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "thisTypeName" ).makeString( m_selfTypeName );
      jsonObjectEncoder.makeMember( "methodName" ).makeString( m_methodName );
      m_params->appendJSON( jsonObjectEncoder.makeMember( "params" ), includeLocation );
    }
    
    std::string MethodOpImpl::getPencilName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = cgManager->getAdapter( m_selfTypeName );
      return CG::MethodPencilName( adapter, m_methodName );
    }
    
    std::string MethodOpImpl::getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = cgManager->getAdapter( m_selfTypeName );
      return CG::MethodDefaultSymbolName(
        CG::ExprType( adapter, CG::USAGE_RVALUE ),
        m_methodName,
        m_params->getExprTypes( cgManager )
        );
    }
    
    RC::ConstHandle<ParamVector> MethodOpImpl::getParams( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return ParamVector::Create(
        Param::Create( getLocation(), "this", m_selfTypeName, !getReturnTypeName().empty() ? CG::USAGE_RVALUE : CG::USAGE_LVALUE ),
        m_params
        );
    }
  }
}
