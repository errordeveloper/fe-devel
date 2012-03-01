/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/AssignOpImpl.h>
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/OpTypes.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( AssignOpImpl );
    
    RC::ConstHandle<AssignOpImpl> AssignOpImpl::Create(
      CG::Location const &location,
      std::string const &thisTypeName,
      CG::AssignOpType assignOpType,
      RC::ConstHandle<AST::Param> rhs,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body
      )
    {
      return new AssignOpImpl( location, thisTypeName, assignOpType, rhs, symbolName, body );
    }
    
    AssignOpImpl::AssignOpImpl(
      CG::Location const &location,
      std::string const &thisTypeName,
      CG::AssignOpType assignOpType,
      RC::ConstHandle<AST::Param> rhs,
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
      , m_assignOpType( assignOpType )
      , m_rhs( rhs )
    {
      FABRIC_ASSERT( m_rhs->getUsage() == CG::USAGE_RVALUE );
    }
    
    void AssignOpImpl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      FunctionBase::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "op" ).makeString( CG::assignOpUserName( m_assignOpType ) );
      m_rhs->appendJSON( jsonObjectEncoder.makeMember( "rhs" ), includeLocation );
    }
    
    std::string AssignOpImpl::getPencilKey( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> thisAdapter = cgManager->getAdapter( m_thisTypeName );
      return CG::AssignOpPencilKey( thisAdapter, m_assignOpType );
    }
    
    std::string AssignOpImpl::getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> thisAdapter = cgManager->getAdapter( m_thisTypeName );
      RC::ConstHandle<CG::Adapter> thatAdapter = cgManager->getAdapter( m_rhs->getType() );
      return CG::AssignOpDefaultSymbolName( thisAdapter, m_assignOpType, thatAdapter );
    }
    
    std::string AssignOpImpl::getDesc( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> thisAdapter = cgManager->getAdapter( m_thisTypeName );
      RC::ConstHandle<CG::Adapter> thatAdapter = cgManager->getAdapter( m_rhs->getType() );
      return CG::AssignOpFullDesc( thisAdapter, m_assignOpType, thatAdapter );
    }

    RC::ConstHandle<ParamVector> AssignOpImpl::getParams( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return ParamVector::Create(
        Param::Create( getLocation(), "this", m_thisTypeName, CG::USAGE_LVALUE ),
        m_rhs
        );
    }
  }
}
