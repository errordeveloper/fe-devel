/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/DG/Binding.h>
#include <Fabric/Core/DG/Prototype.h>
#include <Fabric/Core/DG/BindingList.h>
#include <Fabric/Core/DG/Operator.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace DG
  {
    RC::Handle<Binding> Binding::Create( RC::Handle<Context> const &context )
    {
      return new Binding( context );
    }

    Binding::Binding( RC::Handle<Context> const &context )
      : CompiledObject( context )
      , m_prototype( 0 )
      , m_context( context.ptr() )
    {
    }
    
    Binding::~Binding()
    {
      if ( m_operator )
        m_operator->removeBinding( this );
      FABRIC_ASSERT( m_bindingLists.empty() );
      delete m_prototype;
    }
    
    void Binding::propagateMarkForRecompileImpl( unsigned generation )
    {
      for ( BindingLists::const_iterator it=m_bindingLists.begin(); it!=m_bindingLists.end(); ++it )
        (*it)->markForRecompile( generation );
    }
      
    void Binding::propagateMarkForRefreshImpl( unsigned generation )
    {
      for ( BindingLists::const_iterator it=m_bindingLists.begin(); it!=m_bindingLists.end(); ++it )
        (*it)->markForRefresh( generation );
    }
    
    void Binding::collectErrors()
    {
      Errors &errors = getErrors();
      if ( !m_prototype )
        errors.push_back( "no prototype set" );
      if ( !m_operator )
        errors.push_back( "no operator set" );
    }
    
    void Binding::invalidateRunState()
    {
    }
    
    void Binding::refreshRunState()
    {
    }
    
    void Binding::collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const
    {
    }
    
    bool Binding::canExecute() const
    {
      return m_prototype
        && m_operator;
    }
      
    void Binding::setPrototype( std::vector<std::string> const &parameterDescs )
    {
      Prototype *newPrototype = new Prototype( m_context->getCGManager() );
      newPrototype->setDescs( parameterDescs );
      delete m_prototype;
      m_prototype = newPrototype;
      markForRecompile();
    }
    
    std::vector<std::string> Binding::getPrototype() const
    {
      if ( !m_prototype )
        throw Exception( "no prototype set" );
      return m_prototype->desc();
    }
    
    RC::Handle<Operator> Binding::getOperator() const
    {
      return m_operator;
    }
    
    void Binding::setOperator( RC::Handle<Operator> const &op )
    {
      if ( m_operator )
        m_operator->removeBinding( this );
      m_operator = op;
      if ( m_operator )
        m_operator->addBinding( this );
        
      markForRecompile();
    }
    
    RC::Handle<MT::ParallelCall> Binding::bind(
      std::vector<std::string> &errors,
      Scope const &scope,
      unsigned prefixCount,
      void * const *prefixes
      ) const
    {
      if ( !m_prototype )
        errors.push_back( "no prototype set" );
      if ( !m_operator )
        errors.push_back( "no operator set" );
        
      RC::Handle<MT::ParallelCall> result;
      if ( m_prototype && m_operator )
      {
        std::string const operatorErrorPrefix = "operator " + _(m_operator->getName()) + ": ";
        result = m_operator->bind( errors, m_prototype, scope, prefixCount, prefixes );
        for ( size_t i=0; i<errors.size(); ++i )
          errors[i] = operatorErrorPrefix + errors[i];
      }
      return result;
    }
      
    std::string const &Binding::getOperatorName() const
    {
      if ( !m_operator )
        throw Exception( "no operator set" );
      return m_operator->getName();
    }
    
    void Binding::addBindingList( BindingList *bindingList )
    {
      m_bindingLists.insert( bindingList );
    }
    
    void Binding::removeBindingList( BindingList *bindingList )
    {
      BindingLists::iterator it = m_bindingLists.find( bindingList );
      FABRIC_ASSERT( it != m_bindingLists.end() );
      m_bindingLists.erase( it );
    }
      
    void Binding::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      
      {
        JSON::Encoder parameterLayoutEncoder = resultObjectEncoder.makeMember( "parameterLayout", 15 );
        if ( m_prototype )
          m_prototype->jsonDesc( parameterLayoutEncoder );
        else parameterLayoutEncoder.makeNull();
      }
      
      {
        JSON::Encoder operatorEncoder = resultObjectEncoder.makeMember( "operator", 8 );
        if ( m_operator )
          operatorEncoder.makeString( m_operator->getName() );
        else operatorEncoder.makeNull();
      }
    }
    
    bool Binding::getMainThreadOnly() const
    {
      return m_operator && m_operator->getMainThreadOnly();
    }
  };
};
