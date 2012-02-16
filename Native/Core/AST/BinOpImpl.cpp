/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#include <Fabric/Core/AST/BinOpImpl.h>
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( BinOpImpl );
    
    RC::ConstHandle<BinOpImpl> BinOpImpl::Create(
      CG::Location const &location,
      std::string const &returnTypeName,
      CG::BinOpType binOpType,
      RC::ConstHandle<AST::Param> const &lhs,
      RC::ConstHandle<AST::Param> const &rhs,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body
      )
    {
      return new BinOpImpl( location, returnTypeName, binOpType, lhs, rhs, symbolName, body );
    }
        
    BinOpImpl::BinOpImpl(
      CG::Location const &location,
      std::string const &returnTypeName,
      CG::BinOpType binOpType,
      RC::ConstHandle<AST::Param> const &lhs,
      RC::ConstHandle<AST::Param> const &rhs,
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
      , m_binOpType( binOpType )
      , m_lhs( lhs )
      , m_rhs( rhs )
    {
      FABRIC_ASSERT( m_lhs->getUsage() == CG::USAGE_RVALUE );
      FABRIC_ASSERT( m_rhs->getUsage() == CG::USAGE_RVALUE );
    }
    
    void BinOpImpl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      FunctionBase::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "op" ).makeString( CG::binOpUserName( m_binOpType ) );
      m_lhs->appendJSON( jsonObjectEncoder.makeMember( "lhs" ), includeLocation );
      m_rhs->appendJSON( jsonObjectEncoder.makeMember( "rhs" ), includeLocation );
    }
    
    std::string BinOpImpl::getPencilKey( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return CG::BinOpPencilKey(
        m_binOpType
        );
    }
    
    std::string BinOpImpl::getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return CG::BinOpDefaultSymbolName(
        m_binOpType, 
        cgManager->getAdapter( m_lhs->getType() ),
        cgManager->getAdapter( m_rhs->getType() )
        );
    }
    
    std::string BinOpImpl::getDesc( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return CG::BinOpFullDesc(
        getReturnAdapter( cgManager ),
        m_binOpType, 
        cgManager->getAdapter( m_lhs->getType() ),
        cgManager->getAdapter( m_rhs->getType() )
        );
    }

    RC::ConstHandle<ParamVector> BinOpImpl::getParams( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return ParamVector::Create(
        m_lhs,
        m_rhs
        );
    }
  }
}
