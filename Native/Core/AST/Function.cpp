/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Function.h"
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( Function );
    
    RC::ConstHandle<Function> Function::Create(
      CG::Location const &location,
      std::string const &returnTypeName,
      std::string const &functionName,
      RC::ConstHandle<ParamVector> const &params,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body
      )
    {
      return new Function( location, returnTypeName, functionName, params, symbolName, body, false );
    }
    
    Function::Function(
      CG::Location const &location,
      std::string const &returnTypeName,
      std::string const &functionName,
      RC::ConstHandle<ParamVector> const &params,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body,
      bool exportSymbol
      )
      : FunctionBase( location, returnTypeName, symbolName, body, exportSymbol )
      , m_functionName( functionName )
      , m_params( params )
    {
    }
    
    void Function::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      FunctionBase::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "functionName" ).makeString( m_functionName );
      m_params->appendJSON( jsonObjectEncoder.makeMember( "params" ), includeLocation );
    }
    
    std::string const *Function::getScopeName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = cgManager->maybeGetAdapter( m_functionName );
      if ( adapter )
      {
        FABRIC_ASSERT( getReturnTypeName().empty() );
        return 0;
      }
      else return &m_functionName;
    }
    
    std::string Function::getPencilKey( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = cgManager->maybeGetAdapter( m_functionName );
      if ( adapter )
      {
        FABRIC_ASSERT( getReturnTypeName().empty() );
        return CG::ConstructorPencilKey( adapter );
      }
      else return CG::FunctionPencilKey( m_functionName );
    }
    
    std::string Function::getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = cgManager->maybeGetAdapter( m_functionName );
      if ( adapter )
      {
        FABRIC_ASSERT( getReturnTypeName().empty() );
        return CG::ConstructorDefaultSymbolName(
          adapter,
          m_params->getAdapters( cgManager )
          );
      }
      else return CG::FunctionDefaultSymbolName( m_functionName, m_params->getExprTypes( cgManager ) );
    }
    
    std::string Function::getDesc( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> thisAdapter = cgManager->maybeGetAdapter( m_functionName );
      if ( thisAdapter )
      {
        FABRIC_ASSERT( getReturnTypeName().empty() );
        return CG::ConstructorFullDesc(
          thisAdapter,
          m_params->getAdapters( cgManager )
          );
      }
      else
      {
        return CG::FunctionFullDesc(
          getReturnAdapter( cgManager ),
          m_functionName,
          m_params->getExprTypes( cgManager )
          );
      }
    }

    RC::ConstHandle<ParamVector> Function::getParams( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = cgManager->maybeGetAdapter( m_functionName );
      if ( adapter )
      {
        FABRIC_ASSERT( getReturnTypeName().empty() );
        return ParamVector::Create( Param::Create( getLocation(), "this", m_functionName, CG::USAGE_LVALUE ), m_params );
      }
      else return m_params;
    }
  }
}
