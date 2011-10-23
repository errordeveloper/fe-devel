/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/DG/BindingList.h>
#include <Fabric/Core/DG/Binding.h>
#include <Fabric/Core/DG/NamedObject.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/Operator.h>
#include <Fabric/Base/JSON/Integer.h>
#include <Fabric/Base/JSON/String.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Base/JSON/Array.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Core/Util/Format.h>
#include <Fabric/Core/Util/JSONGenerator.h>

namespace Fabric
{
  namespace DG
  {
    RC::Handle<BindingList> BindingList::Create( RC::Handle<Context> const &context )
    {
      return new BindingList( context );
    }

    BindingList::BindingList( RC::Handle<Context> const &context )
      : CompiledObject( context )
      , m_context( context.ptr() )
      , m_owner( 0 )
    {
    }

    BindingList::~BindingList()
    {
      for ( Bindings::iterator it = m_bindings.begin(); it != m_bindings.end(); ++it )
      {
        RC::Handle<Binding> binding = *it;
        binding->removeBindingList( this );
      }
      
      FABRIC_ASSERT( !m_owner );
    }

    void BindingList::addOwner( NamedObject *owner, std::string const &subName )
    {
      FABRIC_ASSERT( !m_owner );
      m_owner = owner;
      m_subName = subName;
    }
        
    void BindingList::removeOwner( NamedObject *owner )
    {
      FABRIC_ASSERT( m_owner == owner );
      m_owner = 0;
    }
    
    RC::Handle<Binding> BindingList::get( size_t idx )
    {
      if ( idx >= m_bindings.size() )
        throw Exception("index out of range");
      
      Bindings::iterator itOp = m_bindings.begin();
      for ( size_t i=0; i<idx; ++i )
        ++itOp;
      
      return *itOp;
    }

    void BindingList::append( RC::Handle<Binding> const &binding )
    {
      m_bindings.push_back( binding );
      binding->addBindingList( this );
      
      markForRecompile();
      
      jsonNotify();
    }
    
    void BindingList::jsonNotify() const
    {
      std::vector<std::string> src;
      src.push_back( "DG" );
      src.push_back( m_owner->getName() );
      src.push_back( m_subName );
      
      Util::SimpleString json;
      {
        Util::JSONGenerator jg( &json );
        jsonDesc( jg );
      }
      m_context->jsonNotify( src, "delta", 5, &json );
    }

    void BindingList::insert( RC::Handle<Binding> const &binding, size_t beforeIdx )
    {
      if ( beforeIdx > m_bindings.size() )
        throw Exception("index out of range");
      
      Bindings::iterator itOp = m_bindings.begin();
      for ( size_t i=0; i<beforeIdx; ++i )
        ++itOp;
      
      m_bindings.insert( itOp, binding );
      
      binding->addBindingList( this );
      
      markForRecompile();
      
      jsonNotify();
    }

    void BindingList::move( size_t fromIdx, size_t toBeforeIdx )
    {
      if ( fromIdx >= m_bindings.size() )
        throw Exception("source index out of range");
      
      if ( toBeforeIdx > m_bindings.size() )
        throw Exception("destination index out of range");
      
      if ( fromIdx == toBeforeIdx )
        return;
      
      Bindings::iterator itOpFrom = m_bindings.begin();
      for ( size_t i=0; i<fromIdx; ++i )
        ++itOpFrom;
              
      Bindings::iterator itOpTo;
      
      if ( toBeforeIdx < m_bindings.size() )
      {
        itOpTo = m_bindings.begin();
        for ( size_t i=0; i<toBeforeIdx; ++i )
          ++itOpTo;
      }
      else itOpTo = m_bindings.end();
      
      m_bindings.splice( itOpTo, m_bindings, itOpFrom );
      
      markForRecompile();
      
      jsonNotify();
    }

    void BindingList::remove( size_t idx )
    {
      if ( idx >= m_bindings.size() )
        throw Exception("index out of range");
      
      Bindings::iterator itOp = m_bindings.begin();
      for ( size_t i=0; i<idx; ++i )
        ++itOp;
      RC::Handle<Binding> binding = *itOp;
      
      binding->removeBindingList( this );
      
      m_bindings.erase( itOp );
      
      markForRecompile();
      
      jsonNotify();
    }
    
    void BindingList::clear()
    {
      while ( !m_bindings.empty() )
      {
        Bindings::iterator it = m_bindings.begin();
        RC::Handle<Binding> binding = *it;
        m_bindings.erase( it );
        binding->removeBindingList( this );
      }
      markForRecompile();
      
      jsonNotify();
    }
    
    size_t BindingList::size() const
    {
      return m_bindings.size();
    }
    
    void BindingList::propagateMarkForRecompileImpl( unsigned generation )
    {
      if ( m_owner )
        m_owner->markForRecompile( generation );
    }
      
    void BindingList::propagateMarkForRefreshImpl( unsigned generation )
    {
      if ( m_owner )
        m_owner->markForRefresh( generation );
    }
    
    void BindingList::collectErrors()
    {
    }
    
    void BindingList::invalidateRunState()
    {
    }
    
    void BindingList::refreshRunState()
    {
    }
    
    void BindingList::collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const
    {
    }
    
    bool BindingList::canExecute() const
    {
      return true;
    }

    void BindingList::jsonRoute(
      std::vector<std::string> const &dst,
      size_t dstOffset,
      std::string const &cmd,
      RC::ConstHandle<JSON::Value> const &arg,
      Util::JSONArrayGenerator &resultJAG
      )
    {
      if ( dst.size() - dstOffset == 0 )
      {
        try
        {
          jsonExec( cmd, arg, resultJAG );
        }
        catch ( Exception e )
        {
          throw "command " + _(cmd) + ": " + e;
        }
      }
      else throw Exception( "unroutable" );
    }

    void BindingList::jsonExec(
      std::string const &cmd,
      RC::ConstHandle<JSON::Value> const &arg,
      Util::JSONArrayGenerator &resultJAG
      )
    {
      if ( cmd == "append" )
        jsonExecAppend( arg, resultJAG );
      else if ( cmd == "insert" )
        jsonExecInsert( arg, resultJAG );
      else if ( cmd == "remove" )
        jsonExecRemove( arg, resultJAG );
      else throw Exception( "unknown command" );
    }
      
    void BindingList::jsonExecAppend( RC::ConstHandle<JSON::Value> const &arg, Util::JSONArrayGenerator &resultJAG )
    {
      RC::ConstHandle<JSON::Object> argJSONObject = arg->toObject();
    
      RC::Handle<Operator> operator_;
      try
      {
        operator_ = m_context->getOperator( argJSONObject->get( "operatorName" )->toString()->value() );
      }
      catch ( Exception e )
      {
        throw "'operatorName': " + e;
      }
      
      std::vector<std::string> parameterLayout;
      try
      {
        RC::ConstHandle<JSON::Array> parameterLayoutJSONArray = argJSONObject->get("parameterLayout")->toArray();
        size_t parameterLayoutJSONArraySize = parameterLayoutJSONArray->size();
        for ( size_t i=0; i<parameterLayoutJSONArraySize; ++i )
        {
          try
          {
            parameterLayout.push_back( parameterLayoutJSONArray->get(i)->toString()->value() );
          }
          catch ( Exception e )
          {
            throw "index " + _(i) + ": " + e;
          }
        }
      }
      catch ( Exception e )
      {
        throw "'parameterLayout': " + e;
      }
      
      RC::Handle<Binding> binding = Binding::Create( m_context );
      binding->setOperator( operator_ );
      binding->setPrototype( parameterLayout );
      append( binding );
    }
      
    void BindingList::jsonExecInsert( RC::ConstHandle<JSON::Value> const &arg, Util::JSONArrayGenerator &resultJAG )
    {
      RC::ConstHandle<JSON::Object> argJSONObject = arg->toObject();
    
      RC::Handle<Operator> operator_;
      try
      {
        operator_ = m_context->getOperator( argJSONObject->get( "operatorName" )->toString()->value() );
      }
      catch ( Exception e )
      {
        throw "'operatorName': " + e;
      }
      
      std::vector<std::string> parameterLayout;
      try
      {
        RC::ConstHandle<JSON::Array> parameterLayoutJSONArray = argJSONObject->get("parameterLayout")->toArray();
        size_t parameterLayoutJSONArraySize = parameterLayoutJSONArray->size();
        for ( size_t i=0; i<parameterLayoutJSONArraySize; ++i )
        {
          try
          {
            parameterLayout.push_back( parameterLayoutJSONArray->get(i)->toString()->value() );
          }
          catch ( Exception e )
          {
            throw "index " + _(i) + ": " + e;
          }
        }
      }
      catch ( Exception e )
      {
        throw "'parameterLayout': " + e;
      }
      
      size_t beforeIndex;
      try
      {
        beforeIndex = argJSONObject->get( "beforeIndex" )->toInteger()->value();
      }
      catch ( Exception e )
      {
        throw "'beforeIndex': " + e;
      }
      
      RC::Handle<Binding> binding = Binding::Create( m_context );
      binding->setOperator( operator_ );
      binding->setPrototype( parameterLayout );
      insert( binding, beforeIndex );
    }
      
    void BindingList::jsonExecRemove( RC::ConstHandle<JSON::Value> const &arg, Util::JSONArrayGenerator &resultJAG )
    {
      RC::ConstHandle<JSON::Object> argJSONObject = arg->toObject();
          
      size_t index;
      try
      {
        index = argJSONObject->get( "index" )->toInteger()->value();
      }
      catch ( Exception e )
      {
        throw "'index': " + e;
      }
      
      remove( index );
    }
    
    void BindingList::jsonDesc( Util::JSONGenerator &resultJG ) const
    {
      Util::JSONArrayGenerator resultJSONArray = resultJG.makeArray();
      for ( Bindings::const_iterator it=m_bindings.begin(); it!=m_bindings.end(); ++it )
      {
        Util::JSONGenerator elementJG = resultJSONArray.makeElement();
        (*it)->jsonDesc( elementJG );
      }
    }
  };
};
