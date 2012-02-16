/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "Operator.h"
#include "Node.h"
#include "Scope.h"
#include "Prototype.h"
#include "Binding.h"

#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/CodeManager.h>
#include <Fabric/Core/DG/Function.h>
#include <Fabric/Core/AST/GlobalList.h>
#include <Fabric/Core/AST/Function.h>
#include <Fabric/Core/AST/Operator.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/RT/Manager.h>

namespace Fabric
{
  namespace DG
  {
    RC::Handle<Operator> Operator::Create( std::string const &name, RC::Handle<Context> const &context )
    {
      RC::Handle<Operator> operator_ = new Operator( name, context );
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        operator_->jsonDesc( jg );
      }
      operator_->jsonNotifyDelta( json );

      return operator_;
    }

    Operator::Operator( std::string const &name, RC::Handle<Context> const &context )
      : NamedObject( name, context )
      , m_context( context.ptr() )
      , m_mainThreadOnly( false )
    {
    }
    
    Operator::~Operator()
    {
      FABRIC_ASSERT( m_bindings.empty() );
    }
    
    void Operator::addBinding( Binding *binding )
    {
      m_bindings.insert( binding );
    }
    
    void Operator::removeBinding( Binding *binding )
    {
      Bindings::iterator it = m_bindings.find( binding );
      FABRIC_ASSERT( it != m_bindings.end() );
      m_bindings.erase( it );
    }
    
    void Operator::propagateMarkForRecompileImpl( unsigned generation )
    {
      for ( Bindings::const_iterator it=m_bindings.begin(); it!=m_bindings.end(); ++it )
        (*it)->markForRecompile( generation );
    }
      
    void Operator::propagateMarkForRefreshImpl( unsigned generation )
    {
      for ( Bindings::const_iterator it=m_bindings.begin(); it!=m_bindings.end(); ++it )
        (*it)->markForRefresh( generation );
    }
    
    void Operator::collectErrors()
    {
      Errors &errors = getErrors();
      if ( !m_filename.length() )
        errors.push_back( "no source code filename specified" );
      else if ( !m_sourceCode.length() )
        errors.push_back( "no source code loaded" );
      else if ( !m_entryFunctionName.length() )
        errors.push_back( "no entry function specified" );
      else if ( !m_code || m_code->getDiagnostics().containsError() )
        errors.push_back( "compile failed" );
      else if ( !m_function )
        errors.push_back( "entry function not found" );
    }
    
    void Operator::invalidateRunState()
    {
    }
    
    void Operator::refreshRunState()
    {
    }
    
    void Operator::collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const
    {
    }
    
    bool Operator::canExecute() const
    {
      return m_filename.length() > 0
        && m_sourceCode.length() > 0
        && m_entryFunctionName.length() > 0
        && m_code
        && !m_code->getDiagnostics().containsError()
        && m_function;
    }
    
    void Operator::compile()
    {
      FABRIC_ASSERT( m_filename.length() > 0 );
      FABRIC_ASSERT( m_sourceCode.length() > 0 );
      FABRIC_ASSERT( !m_code );
      FABRIC_ASSERT( !m_function );
      
      m_code = m_context->getCodeManager()->compileSourceCode( m_context, m_filename, m_sourceCode );
      
      if ( !m_code->getDiagnostics().containsError() )
      {
        if ( m_entryFunctionName.length() > 0 )
        {
          resolveASTOperator();
          resolveEntryFunction();
        }
      }
      
      markForRecompile();
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescDiagnostics( jg );
      }
      jsonNotifyMemberDelta( "diagnostics", 11, json );
    }
    
    void Operator::resolveEntryFunction()
    {
      FABRIC_ASSERT( m_code );
      FABRIC_ASSERT( !m_code->getDiagnostics().containsError() );
      FABRIC_ASSERT( m_entryFunctionName.length() > 0 );
      FABRIC_ASSERT( m_astOperator );
      
      m_function = Function::Create( m_code, m_astOperator->getSymbolName( m_context->getCGManager() ) );
      
      markForRecompile();
    }
    
    RC::Handle<MT::ParallelCall> Operator::bind(
      std::vector<std::string> &errors,
      Prototype *prototype,
      Scope const &scope,
      unsigned prefixCount,
      void * const *prefixes
      ) const
    {
      if ( !m_filename.length() )
        errors.push_back( "no source code filename specified" );
      if ( !m_sourceCode.length() )
        errors.push_back( "no source code loaded" );
      if ( !m_entryFunctionName.length() )
        errors.push_back( "no entry function specified" );
      if ( m_sourceCode.length() && (!m_code || m_code->getDiagnostics().containsError()) )
        errors.push_back( "compile failed" );
      if ( m_entryFunctionName.length() && m_code && !m_code->getDiagnostics().containsError() && !m_function )
        errors.push_back( "entry function not found" );
      
      RC::Handle<MT::ParallelCall> result;
      if ( m_astOperator && m_function )
        result = prototype->bind( errors, m_astOperator, scope, m_function, prefixCount, prefixes );
      return result;
    }
    
    std::string const &Operator::getEntryFunctionName() const
    {
      return m_entryFunctionName;
    }
    
    void Operator::setEntryFunctionName( std::string const &entryFunctionName )
    {
      if ( entryFunctionName != m_entryFunctionName )
      {
        m_function = 0;
        
        m_entryFunctionName = entryFunctionName;
        
        if ( m_entryFunctionName.length() > 0
          && m_code
          && !m_code->getDiagnostics().containsError() )
        {
          resolveASTOperator();
          resolveEntryFunction();
        }
        
        Util::SimpleString json;
        {
          JSON::Encoder jg( &json );
          jsonDescEntryFunctionName( jg );
        }
        jsonNotifyMemberDelta( "entryFunctionName", 17, json );
      }
    }
    
    std::string const &Operator::getFilename() const
    {
      return m_filename;
    }
    
    std::string const &Operator::getSourceCode() const
    {
      return m_sourceCode;
    }
    
#if defined(FABRIC_BUILD_DEBUG)      
    std::string const &Operator::getByteCode() const
    {
      static std::string const empty;
      if ( !m_code )
        return empty;
      return m_code->getByteCode();
    }
#endif

    void Operator::setFilenameAndSourceCode( std::string const &filename, std::string const &sourceCode )
    {
      if ( filename != m_filename || sourceCode != m_sourceCode )
      {
        m_function = 0;
        m_astOperator = 0;
        m_code = 0;
        
        m_filename = filename;
        m_sourceCode = sourceCode;

        if ( m_filename.length() > 0 && m_sourceCode.length() > 0 )
          compile();
        
        {
          Util::SimpleString json;
          {
            JSON::Encoder jg( &json );
            jsonDescFilename( jg );
          }
          jsonNotifyMemberDelta( "filename", 8, json );
        }
        
        {
          Util::SimpleString json;
          {
            JSON::Encoder jg( &json );
            jsonDescSourceCode( jg );
          }
          jsonNotifyMemberDelta( "sourceCode", 10, json );
        }
      }
    }
    
    RC::ConstHandle<AST::GlobalList> Operator::getAST() const
    {
      if ( !m_code )
        throw Exception( "operator is not compiled" );
      return m_code->getAST();
    }
    
    CG::Diagnostics const &Operator::getDiagnostics() const
    {
      if ( !m_code )
        throw Exception( "operator is not compiled" );
      return m_code->getDiagnostics();
    }
    
    void Operator::resolveASTOperator()
    {
      FABRIC_ASSERT( !m_astOperator );
      FABRIC_ASSERT( m_entryFunctionName.length() > 0 );
      FABRIC_ASSERT( m_code );

      RC::ConstHandle<AST::GlobalList> ast = m_code->getAST();
      std::vector< RC::ConstHandle<AST::FunctionBase> > functionBases;
      ast->collectFunctionBases( functionBases );
      for ( std::vector< RC::ConstHandle<AST::FunctionBase> >::const_iterator it=functionBases.begin(); it!=functionBases.end(); ++it )
      {
        RC::ConstHandle<AST::FunctionBase> const &functionBase = *it;
        
        if ( functionBase->isFunction() )
        {
          RC::ConstHandle<AST::Function> function = RC::ConstHandle<AST::Function>::StaticCast( functionBase );
          
          std::string const &declaredName = function->getDeclaredName();
          if ( declaredName == m_entryFunctionName )
          {
            if( !function->isOperator() )
              throw Exception( "attempting to use a function " + _(m_entryFunctionName) + " instead of an operator" );
            m_astOperator = RC::ConstHandle<AST::Operator>::StaticCast( function );
          }
        }
      }

      if ( !m_astOperator )
        throw Exception( "missing operator " + _(m_entryFunctionName) );
    }
      
    void Operator::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      NamedObject::jsonDesc( resultEncoder );
    }
      
    void Operator::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      NamedObject::jsonDesc( resultObjectEncoder );
      
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "filename", 8 );
        jsonDescFilename( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "sourceCode", 10 );
        jsonDescSourceCode( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "entryFunctionName", 17 );
        jsonDescEntryFunctionName( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "diagnostics", 11 );
        jsonDescDiagnostics( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "mainThreadOnly", 14 );
        jsonDescMainThreadOnly( memberEncoder );
      }
    }

    void Operator::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "setEntryFunctionName", 20 ) )
        jsonExecSetEntryFunctionName( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "setSourceCode", 13 ) )
        jsonExecSetSourceCode( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "setMainThreadOnly", 17 ) )
        jsonExecSetMainThreadOnly( arg, resultArrayEncoder );
      else NamedObject::jsonExec( cmd, arg, resultArrayEncoder );
    }
    
    void Operator::jsonExecCreate( JSON::Entity const &arg, RC::Handle<Context> const &context, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      Create( arg.stringToStdString(), context );
    }
      
    void Operator::jsonExecSetSourceCode( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      std::string filename;
      std::string sourceCode;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "filename", 8 ) )
          {
            valueEntity.requireString();
            filename = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "sourceCode", 10 ) )
          {
            valueEntity.requireString();
            sourceCode = valueEntity.stringToStdString();
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( filename.empty() )
        throw Exception( "missing 'filename'" );
      if ( sourceCode.empty() )
        throw Exception( "missing 'sourceCode'" );
        
      setFilenameAndSourceCode( filename, sourceCode );
    }
      
    void Operator::jsonExecSetEntryFunctionName( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      setEntryFunctionName( arg.stringToStdString() );
    }
    
    void Operator::jsonExecSetMainThreadOnly( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireBoolean();
      setMainThreadOnly( arg.booleanValue() );
    }
    
    void Operator::jsonDescType( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeString( "Operator", 8 );
    }
    
    void Operator::jsonDescFilename( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeString( m_filename );
    }
    
    void Operator::jsonDescSourceCode( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeString( m_sourceCode );
    }
    
    void Operator::jsonDescEntryFunctionName( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeString( m_entryFunctionName );
    }
    
    void Operator::jsonDescDiagnostics( JSON::Encoder &resultEncoder ) const
    {
      if ( m_code )
        m_code->getDiagnostics().encodeJSON( resultEncoder );
      else
        resultEncoder.makeArray();
    }
    
    void Operator::jsonDescMainThreadOnly( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeBoolean( m_mainThreadOnly );
    }
  }
}
