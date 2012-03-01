/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/MethodOpImpl.h>
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/Mangling.h>
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
      , m_thisTypeName( thisTypeName )
      , m_thisUsage( !returnTypeName.empty() ? CG::USAGE_RVALUE : CG::USAGE_LVALUE )
      , m_methodName( methodName )
      , m_params( params )
    {
    }
    
    void MethodOpImpl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      FunctionBase::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "thisTypeName" ).makeString( m_thisTypeName );
      jsonObjectEncoder.makeMember( "methodName" ).makeString( m_methodName );
      m_params->appendJSON( jsonObjectEncoder.makeMember( "params" ), includeLocation );
    }
    
    std::string MethodOpImpl::getPencilKey( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = cgManager->getAdapter( m_thisTypeName );
      return CG::MethodPencilKey( adapter, m_methodName );
    }
    
    std::string MethodOpImpl::getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> thisAdapter = cgManager->getAdapter( m_thisTypeName );
      return CG::MethodDefaultSymbolName(
        CG::ExprType( thisAdapter, m_thisUsage ),
        m_methodName,
        m_params->getExprTypes( cgManager )
        );
    }
    
    std::string MethodOpImpl::getDesc( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> returnAdapter = getReturnAdapter( cgManager );
      RC::ConstHandle<CG::Adapter> thisAdapter = cgManager->getAdapter( m_thisTypeName );
      return CG::MethodFullDesc(
        returnAdapter,
        CG::ExprType( thisAdapter, m_thisUsage ),
        m_methodName,
        m_params->getExprTypes( cgManager )
        );
    }
    
    RC::ConstHandle<ParamVector> MethodOpImpl::getParams( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return ParamVector::Create(
        Param::Create( getLocation(), "this", m_thisTypeName, m_thisUsage ),
        m_params
        );
    }
  }
}
