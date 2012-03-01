/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/BindingList.h>
#include <Fabric/Core/DG/Binding.h>
#include <Fabric/Core/DG/NamedObject.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/Operator.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

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
        JSON::Encoder jg( &json );
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
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( dst.size() == dstOffset )
        jsonExec( cmd, arg, resultArrayEncoder );
      else throw Exception( "unroutable" );
    }

    void BindingList::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "append", 6 ) )
        jsonExecAppend( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "insert", 6 ) )
        jsonExecInsert( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "remove", 6 ) )
        jsonExecRemove( arg, resultArrayEncoder );
      else throw Exception( "unknown command" );
    }
      
    void BindingList::jsonExecAppend( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      RC::Handle<Operator> operator_;
      std::vector<std::string> parameterLayout;

      arg.requireObject();
      JSON::ObjectDecoder objectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( objectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "operatorName", 12 ) )
          {
            valueEntity.requireString();
            operator_ = m_context->getOperator( valueEntity.stringToStdString() );
          }
          else if ( keyString.stringIs( "parameterLayout", 15 ) )
          {
            valueEntity.requireArray();
            JSON::ArrayDecoder arrayDecoder( valueEntity );
            JSON::Entity elementEntity;
            while ( arrayDecoder.getNext( elementEntity ) )
            {
              try
              {
                elementEntity.requireString();
                parameterLayout.push_back( elementEntity.stringToStdString() );
              }
              catch ( Exception e )
              {
                throw "index " + _(parameterLayout.size()) + ": " + e;
              }
            }
          }
        }
        catch ( Exception e )
        {
          objectDecoder.rethrow( e );
        }
      }
      
      RC::Handle<Binding> binding = Binding::Create( m_context );
      binding->setOperator( operator_ );
      binding->setPrototype( parameterLayout );
      append( binding );
    }
      
    void BindingList::jsonExecInsert( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      RC::Handle<Operator> operator_;
      std::vector<std::string> parameterLayout;
      size_t beforeIndex;

      arg.requireObject();
      JSON::ObjectDecoder objectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( objectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "operatorName", 12 ) )
          {
            valueEntity.requireString();
            operator_ = m_context->getOperator( valueEntity.stringToStdString() );
          }
          else if ( keyString.stringIs( "parameterLayout", 15 ) )
          {
            valueEntity.requireArray();
            JSON::ArrayDecoder arrayDecoder( valueEntity );
            JSON::Entity elementEntity;
            while ( arrayDecoder.getNext( elementEntity ) )
            {
              try
              {
                elementEntity.requireString();
                parameterLayout.push_back( elementEntity.stringToStdString() );
              }
              catch ( Exception e )
              {
                throw "index " + _(parameterLayout.size()) + ": " + e;
              }
            }
          }
          else if ( keyString.stringIs( "beforeIndex", 11 ) )
          {
            valueEntity.requireInteger();
            beforeIndex = valueEntity.integerValue();
          }
        }
        catch ( Exception e )
        {
          objectDecoder.rethrow( e );
        }
      }
      
      RC::Handle<Binding> binding = Binding::Create( m_context );
      binding->setOperator( operator_ );
      binding->setPrototype( parameterLayout );
      insert( binding, beforeIndex );
    }
      
    void BindingList::jsonExecRemove( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      size_t index;

      arg.requireObject();
      JSON::ObjectDecoder objectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( objectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "index", 5 ) )
          {
            valueEntity.requireInteger();
            index = valueEntity.integerValue();
          }
        }
        catch ( Exception e )
        {
          objectDecoder.rethrow( e );
        }
      }
      
      remove( index );
    }
    
    void BindingList::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      JSON::ArrayEncoder resultArrayEncoder = resultEncoder.makeArray();
      for ( Bindings::const_iterator it=m_bindings.begin(); it!=m_bindings.end(); ++it )
      {
        JSON::Encoder elementEncoder = resultArrayEncoder.makeElement();
        (*it)->jsonDesc( elementEncoder );
      }
    }
  };
};
