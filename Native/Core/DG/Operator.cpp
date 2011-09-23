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
#include <Fabric/Base/JSON/Null.h>
#include <Fabric/Base/JSON/Integer.h>
#include <Fabric/Base/JSON/String.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Base/JSON/Array.h>
#include <Fabric/Base/JSON/Boolean.h>

namespace Fabric
{
  namespace DG
  {
    RC::Handle<Operator> Operator::Create( std::string const &name, RC::Handle<Context> const &context )
    {
      RC::Handle<Operator> operator_ = new Operator( name, context );
      operator_->notifyDelta( operator_->jsonDesc() );
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
      notifyDelta( "diagnostics", jsonDescDiagnostics() );
      
      if ( !m_code->getDiagnostics().containsError() )
      {
        if ( m_entryFunctionName.length() > 0 )
        {
          resolveASTOperator();
          resolveEntryFunction();
        }
      }
      
      markForRecompile();
    }
    
    void Operator::resolveEntryFunction()
    {
      FABRIC_ASSERT( m_code );
      FABRIC_ASSERT( !m_code->getDiagnostics().containsError() );
      FABRIC_ASSERT( m_entryFunctionName.length() > 0 );
      
      m_function = Function::Create( m_code, m_entryFunctionName );
      
      markForRecompile();
    }
    
    RC::Handle<MT::ParallelCall> Operator::bind(
      std::vector<std::string> &errors,
      Prototype *prototype,
      Scope const &scope,
      size_t *newSize,
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
        result = prototype->bind( errors, m_astOperator, scope, m_function, newSize, prefixCount, prefixes );
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
        
        notifyDelta( "entryFunctionName", jsonDescEntryFunctionName() );
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
        notifyDelta( "filename", jsonDescFilename() );
        m_sourceCode = sourceCode;
        notifyDelta( "sourceCode", jsonDescSourceCode() );

        if ( m_filename.length() > 0 && m_sourceCode.length() > 0 )
          compile();
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
      std::vector< RC::ConstHandle<AST::Function> > functions;
      ast->collectFunctions( functions );
      for ( std::vector< RC::ConstHandle<AST::Function> >::const_iterator it=functions.begin(); it!=functions.end(); ++it )
      {
        RC::ConstHandle<AST::Function> const &function = *it;
        
        std::string const *friendlyName = function->getFriendlyName( m_context->getCGManager() );
        if ( friendlyName && *friendlyName == m_entryFunctionName )
        {
          if( !function->isOperator() )
            throw Exception( "attempting to use a function " + _(m_entryFunctionName) + " instead of an operator" );
          m_astOperator = RC::ConstHandle<AST::Operator>::StaticCast( function );
        }
      }

      if ( !m_astOperator )
        throw Exception( "missing operator " + _(m_entryFunctionName) );
    }
      
    RC::Handle<JSON::Object> Operator::jsonDesc() const
    {
      RC::Handle<JSON::Object> result = NamedObject::jsonDesc();
      result->set( "filename", jsonDescFilename() );
      result->set( "sourceCode", jsonDescSourceCode() );
      result->set( "entryFunctionName", jsonDescEntryFunctionName() );
      result->set( "diagnostics", jsonDescDiagnostics() );
      result->set( "mainThreadOnly", jsonDescMainThreadOnly() );
      return result;
    }

    RC::ConstHandle<JSON::Value> Operator::jsonExec( std::string const &cmd, RC::ConstHandle<JSON::Value> const &arg )
    {
      RC::ConstHandle<JSON::Value> result;

      if ( cmd == "setEntryFunctionName" )
        jsonExecSetEntryFunctionName( arg );
      else if ( cmd == "setSourceCode" )
        jsonExecSetSourceCode( arg );
      else if ( cmd == "setMainThreadOnly" )
        jsonExecSetMainThreadOnly( arg );
      else result = NamedObject::jsonExec( cmd, arg );
      
      return result;
    }
    
    void Operator::jsonExecCreate( RC::ConstHandle<JSON::Value> const &arg, RC::Handle<Context> const &context )
    {
      Create( arg->toString()->value(), context );
    }
      
    void Operator::jsonExecSetSourceCode( RC::ConstHandle<JSON::Value> const &arg )
    {
      RC::ConstHandle<JSON::Object> argObject = arg->toObject();
      
      std::string filename;
      try
      {
        filename = argObject->get("filename")->toString()->value();
      }
      catch ( Exception e )
      {
        throw "filename: " + e;
      }
      
      std::string sourceCode;
      try
      {
        sourceCode = argObject->get("sourceCode")->toString()->value();
      }
      catch ( Exception e )
      {
        throw "sourceCode: " + e;
      }
        
      setFilenameAndSourceCode( filename, sourceCode );
    }
      
    void Operator::jsonExecSetEntryFunctionName( RC::ConstHandle<JSON::Value> const &arg )
    {
      setEntryFunctionName( arg->toString()->value() );
    }
    
    void Operator::jsonExecSetMainThreadOnly( RC::ConstHandle<JSON::Value> const &arg )
    {
      setMainThreadOnly( arg->toBoolean()->value() );
    }
    
    RC::ConstHandle<JSON::Value> Operator::jsonDescType() const
    {
      static RC::ConstHandle<JSON::Value> result = JSON::String::Create( "Operator" );
      return result;
    }
    
    RC::ConstHandle<JSON::Value> Operator::jsonDescFilename() const
    {
      return JSON::String::Create( m_filename );
    }
    
    RC::ConstHandle<JSON::Value> Operator::jsonDescSourceCode() const
    {
      return JSON::String::Create( m_sourceCode );
    }
    
    RC::ConstHandle<JSON::Value> Operator::jsonDescEntryFunctionName() const
    {
      return JSON::String::Create( m_entryFunctionName );
    }
    
    RC::ConstHandle<JSON::Value> Operator::jsonDescDiagnostics() const
    {
      RC::Handle<JSON::Array> result = JSON::Array::Create();
      if ( m_code )
      {
        CG::Diagnostics const &diagnostics = m_code->getDiagnostics();
        for ( CG::Diagnostics::const_iterator it=diagnostics.begin(); it!=diagnostics.end(); ++it )
        {
          CG::Location const &location = it->first;
          CG::Diagnostic const &diagnostic = it->second;
          
          RC::Handle<JSON::Object> sub = JSON::Object::Create();
          sub->set( "filename", JSON::String::Create( location.getFilename() ) );
          sub->set( "line", JSON::Integer::Create( location.getLine() ) );
          sub->set( "column", JSON::Integer::Create( location.getColumn() ) );
          sub->set( "level", JSON::String::Create( diagnostic.getLevelDesc() ) );
          sub->set( "desc", JSON::String::Create( diagnostic.getDesc().getData(), diagnostic.getDesc().getLength() ) );
          
          result->push_back( sub );
        }
      }
      return result;
    }
    
    RC::ConstHandle<JSON::Value> Operator::jsonDescMainThreadOnly() const
    {
      return JSON::Boolean::Create( m_mainThreadOnly );
    }
  };
};

