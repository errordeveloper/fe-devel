/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/GlobalList.h>
#include <Fabric/Core/AST/Function.h>
#include <Fabric/Core/AST/Global.h>
#include <Fabric/Core/CG/Diagnostics.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    RC::ConstHandle<GlobalList> GlobalList::Create( RC::ConstHandle<Global> const &first, RC::ConstHandle<GlobalList> const &remaining )
    {
      return new GlobalList( 0, first, remaining );
    }
    
    RC::ConstHandle<GlobalList> GlobalList::Create( RC::ConstHandle<GlobalList> const &lhs, RC::ConstHandle<GlobalList> const &rhs )
    {
      return new GlobalList( lhs, 0, rhs );
    }
    
    GlobalList::GlobalList( RC::ConstHandle<GlobalList> const &before, RC::ConstHandle<Global> const &global, RC::ConstHandle<GlobalList> const &after )
      : m_before( before )
      , m_global( global )
      , m_after( after )
    {
    }
    
    Util::SimpleString GlobalList::toJSON( bool includeLocation ) const
    {
      Util::SimpleString ss;
      {
        JSON::Encoder encoder( &ss );
        encodeJSON( includeLocation, encoder );
      }
      return ss;
    }
    
    void GlobalList::encodeJSON( bool includeLocation, JSON::Encoder &jg ) const
    {
      JSON::ArrayEncoder jsonArrayEncoder = jg.makeArray();
      appendJSON( jsonArrayEncoder, includeLocation );
    }
    
    void GlobalList::appendJSON( JSON::ArrayEncoder const &jsonArrayEncoder, bool includeLocation ) const
    {
      if ( m_before )
        m_before->appendJSON( jsonArrayEncoder, includeLocation );
      if ( m_global )
        m_global->appendJSON( jsonArrayEncoder.makeElement(), includeLocation );
      if ( m_after )
        m_after->appendJSON( jsonArrayEncoder, includeLocation );
    }
    
    void GlobalList::collectRequires( RequireNameToLocationMap &requires ) const
    {
      if ( m_before )
        m_before->collectRequires( requires );
      if ( m_global )
        m_global->collectRequires( requires );
      if ( m_after )
        m_after->collectRequires( requires );
    }
    
    void GlobalList::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      if ( m_before )
        m_before->registerTypes( cgManager, diagnostics );
      if ( m_global )
      {
        try
        {
          m_global->registerTypes( cgManager, diagnostics );
        }
        catch ( CG::Error e )
        {
          diagnostics.addError( e.getLocation(), e.getDesc() );
        }
      }
      if ( m_after )
        m_after->registerTypes( cgManager, diagnostics );
    }
    
    void GlobalList::llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctions ) const
    {
      if ( m_before )
        m_before->llvmCompileToModule( moduleBuilder, diagnostics, buildFunctions );
      if ( m_global )
      {
        try
        {
          m_global->llvmCompileToModule( moduleBuilder, diagnostics, buildFunctions );
        }
        catch ( CG::Error e )
        {
          diagnostics.addError( e.getLocation(), e.getDesc() );
        }
      }
      if ( m_after )
        m_after->llvmCompileToModule( moduleBuilder, diagnostics, buildFunctions );
    }

    void GlobalList::collectFunctionBases( std::vector< RC::ConstHandle<AST::FunctionBase> > &result ) const
    {
      if ( m_before )
        m_before->collectFunctionBases( result );
      if ( m_global && m_global->isFunctionBase() )
        result.push_back( RC::ConstHandle<AST::FunctionBase>::StaticCast( m_global ) );
      if ( m_after )
        m_after->collectFunctionBases( result );
    }
  }
}
