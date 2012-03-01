/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "EventHandler.h"
#include "Event.h"
#include "Node.h"
#include "Binding.h"
#include "BindingList.h"
#include "Scope.h"
#include "Operator.h"
#include "SelectedNode.h"
#include <Fabric/Core/DG/Context.h>

namespace Fabric
{
  namespace DG
  {
    EventHandlerTask::EventHandlerTask( EventHandler *t, MT::Task<EventHandler>::ExecuteCallback executeCallback )
      : MT::Task<EventHandler>( t, executeCallback )
      , m_context( t->m_context )
    {
      m_shouldSelect = false;
      m_selectParallelCall = 0;
    }
    
    EventHandlerTask::EventHandlerTask( EventHandler *t, MT::Task<EventHandler>::ExecuteCallback executeCallback, RC::Handle<Node> const &node, RC::ConstHandle<RT::Desc> const &selectorType )
      : MT::Task<EventHandler>( t, executeCallback )
      , m_selectedNode( node, selectorType )
      , m_context( t->m_context )
    {
      m_shouldSelect = false;
      m_selectParallelCall = 0;
    }
    
    void EventHandlerTask::execute( void *userdata ) const
    {
      size_t numBindings = m_evaluateParallelCallsPerOperator.size();
      for ( size_t i=0; i<numBindings; ++i )
      {
        RC::Handle<MT::ParallelCall> opParallelCall = m_evaluateParallelCallsPerOperator[i];
        opParallelCall->executeSerial( m_context );
      }
      
      SelectedNodeList *selectedNodes = (SelectedNodeList*)userdata;
      if(m_selectParallelCall && selectedNodes )
      {
        m_selectParallelCall->executeSerial( m_context );
        if ( m_shouldSelect ){
          selectedNodes->push_back( m_selectedNode );
        }
      }
    }
    
    RC::Handle<EventHandler> EventHandler::Create( std::string const &name, RC::Handle<Context> const &context )
    {
      RC::Handle<EventHandler> eventHandler = new EventHandler( name, context );

      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        eventHandler->jsonDesc( jg );
      }
      eventHandler->jsonNotifyDelta( json );
      
      return eventHandler;
    }
    
    EventHandler::EventHandler( std::string const &name, RC::Handle<Context> const &context )
      : Container( name, context )
      , m_context( context.ptr() )
      , m_runState( 0 )
    {
      m_preDescendBindings = BindingList::Create( context );
      m_preDescendBindings->addOwner( this, "preDescendBindings" );
      
      m_postDescendBindings = BindingList::Create( context );
      m_postDescendBindings->addOwner( this, "postDescendBindings" );
    }
    
    EventHandler::~EventHandler()
    {
      delete m_runState;
      
      FABRIC_ASSERT( m_events.empty() );
      
      while ( !m_bindings.empty() )
      {
        Util::UnorderedMap< std::string, RC::Handle<Node> >::iterator it = m_bindings.begin();
        RC::Handle<Node> node = it->second;
        FABRIC_ASSERT( node );
        m_bindings.erase( it );
        
        Node::EventHandlers::iterator jt = node->m_eventHandlers.find( this );
        FABRIC_ASSERT( jt != node->m_eventHandlers.end() );
        node->m_eventHandlers.erase( jt );
      }
      
      FABRIC_ASSERT( m_parentEventHandlers.empty() );
      
      for ( size_t i=0; i<m_childEventHandlers.size(); ++i )
      {
        RC::Handle<EventHandler> const &childEventHandler = m_childEventHandlers[i];
        childEventHandler->removeParent( this );
      }
      
      m_postDescendBindings->removeOwner( this );
      m_preDescendBindings->removeOwner( this );
    }
      
    void EventHandler::addEvent( Event *event )
    {
      FABRIC_VERIFY( m_events.insert( event ).second );
    }
    
    void EventHandler::removeEvent( Event *event )
    {
      std::set< Event * >::iterator it = m_events.find( event );
      FABRIC_ASSERT( it != m_events.end() );
      m_events.erase( it );
    }
      
    void EventHandler::addParent( EventHandler *eventHandler )
    {
      m_parentEventHandlers.insert( eventHandler );
    }
    
    void EventHandler::removeParent( EventHandler *eventHandler )
    {
      ParentEventHandlers::iterator it = m_parentEventHandlers.find( eventHandler );
      FABRIC_ASSERT( it != m_parentEventHandlers.end() );
      m_parentEventHandlers.erase( it );
    }
      
    void EventHandler::appendChildEventHandler( RC::Handle<EventHandler> const &eventHandler )
    {
      if ( !eventHandler )
        throw Exception( "event handler is null" );
      m_childEventHandlers.push_back( eventHandler );
      eventHandler->addParent( this );
          
      markForRecompile();
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescChildEventHandlers( jg );
      }
      jsonNotifyMemberDelta( "childEventHandlers", 18, json );
    }
    
    EventHandler::ChildEventHandlers const &EventHandler::getChildEventHandlers() const
    {
      return m_childEventHandlers;
    }
      
    void EventHandler::removeChildEventHandler( RC::Handle<EventHandler> const &eventHandler )
    {
      size_t numRemoved = 0;
      for ( size_t i=0; i<m_childEventHandlers.size(); )
      {
        if ( m_childEventHandlers[i] == eventHandler )
        {
          m_childEventHandlers[i]->removeParent( this );
          for ( size_t j=i+1; j<m_childEventHandlers.size(); ++j )
            m_childEventHandlers[j-1] = m_childEventHandlers[j];
          m_childEventHandlers.resize( m_childEventHandlers.size() - 1 );
          ++numRemoved;
        }
        else ++i;
      }
      if ( numRemoved == 0 )
        throw Exception( "child event handler not found" );
          
      markForRecompile();
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescChildEventHandlers( jg );
      }
      jsonNotifyMemberDelta( "childEventHandlers", 18, json );
    }

    EventHandler::ParentEventHandlers const &EventHandler::getParentEventHandlers() const
    {
      return m_parentEventHandlers;
    }

    void EventHandler::setScope( std::string const &name, RC::Handle<Node> const &node )
    {
      if ( name.empty() )
        throw Exception( "name must be non-empty" );
      
      RC::Handle<Node> &bindingSlot = m_bindings[name];
      if ( bindingSlot != node )
      {
        if ( bindingSlot )
          bindingSlot->removeEventHandler( this );
        bindingSlot = node;
        if ( bindingSlot )
          bindingSlot->addEventHandler( this );
          
        markForRecompile();
        
        Util::SimpleString json;
        {
          JSON::Encoder jg( &json );
          jsonDescScopes( jg );
        }
        jsonNotifyMemberDelta( "scopes", 6, json );
      }
    }

    void EventHandler::removeScope( std::string const &name )
    {
      if ( name.empty() )
        throw Exception( "name must be non-empty" );
      
      ExternalScopes::iterator it = m_bindings.find(name);
      if ( it == m_bindings.end() )
        throw Exception( _(name) + ": no such scope" );
      it->second->removeEventHandler( this );
      m_bindings.erase( it );
        
      markForRecompile();
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescScopes( jg );
      }
      jsonNotifyMemberDelta( "scopes", 6, json );
    }

    EventHandler::Bindings const &EventHandler::getScopes() const
    {
      return m_bindings;
    }
      
    std::string const &EventHandler::getScopeName() const
    {
      return m_bindingName;
    }
    
    void EventHandler::setScopeName( std::string const &bindingName )
    {
      if ( bindingName != m_bindingName )
      {
        m_bindingName = bindingName;

        markForRecompile();
        
        Util::SimpleString json;
        {
          JSON::Encoder jg( &json );
          jsonDescBindingName( jg );
        }
        jsonNotifyMemberDelta( "bindingName", 11, json );
      }
    }
    
    RC::Handle<BindingList> EventHandler::getPreDescendBindingList( ) const
    {
      return m_preDescendBindings;
    }
    
    RC::Handle<BindingList> EventHandler::getPostDescendBindingList( ) const
    {
      return m_postDescendBindings;
    }
    
    void EventHandler::setPreDescendBindingList( RC::Handle<BindingList> opList )
    {
      if( opList.isNull() )
        throw Exception( "no operator list given" );
      
      m_preDescendBindings->removeOwner( this );
      opList->addOwner( this, "preDescendBindings" );
      m_preDescendBindings = opList;
    }
    
    void EventHandler::setPostDescendBindingList( RC::Handle<BindingList> opList )
    {
      if( opList.isNull() )
        throw Exception( "no operator list given" );
      
      m_postDescendBindings->removeOwner( this );
      opList->addOwner( this, "postDescendBindings" );
      m_postDescendBindings = opList;
    }
        
    void EventHandler::setSelector( std::string const &nodeBindingName, RC::Handle<Binding> const &binding )
    {
      if ( nodeBindingName.empty() )
        throw Exception( "nodeBindingName: must be non-empty" );
      m_selectNodeBindingName = nodeBindingName;
      m_selectBinding = binding;
      markForRecompile();
    }
    
    void EventHandler::fire( Scope const *parentScope, RC::ConstHandle<RT::Desc> const &selectorType, SelectedNodeList *selectedNodes )
    {
    }
    
    void EventHandler::collectEventTasksImpl( EventTaskGroup &taskGroup, RC::ConstHandle<RT::Desc> const &selectorType, Scope const *parentScope )
    {
      BindingsScope bindingsScope( m_bindings, parentScope );
      SelfScope selfScope( this, &bindingsScope );
      NamedScope bindingNameScope( m_bindingName, this, &bindingsScope );
      Scope *childScope = m_bindingName.length()>0? (Scope *)&bindingNameScope: (Scope *)&bindingsScope;
      
      size_t numPreDescendBindings = m_preDescendBindings->size();
      if ( numPreDescendBindings > 0 )
      {
        EventHandlerTask* task = new EventHandlerTask( this, &EventHandler::evaluateLocal );
        
        task->m_evaluateParallelCallsPerOperator.resize(numPreDescendBindings);
        
        for ( size_t i=0; i<numPreDescendBindings; ++i )
        {
          RC::Handle<Binding> binding = m_preDescendBindings->get(i);
          std::vector<std::string> errors;
          task->m_evaluateParallelCallsPerOperator[ i ] = binding->bind( errors, selfScope );
        }
        taskGroup.add(task);
      }
      
      for ( size_t i=0; i<m_childEventHandlers.size(); ++i )
      {
        RC::Handle<EventHandler> const &childEventHandler = m_childEventHandlers[i];
        childEventHandler->collectEventTasksImpl( taskGroup, selectorType, childScope );
      }
      
      size_t numPostDescendBindings = m_postDescendBindings->size();
      if ( numPostDescendBindings > 0 )
      {
        EventHandlerTask* task = new EventHandlerTask( this, &EventHandler::evaluateLocal );
        task->m_evaluateParallelCallsPerOperator.resize(numPostDescendBindings);
        
        for ( size_t i=0; i<numPostDescendBindings; ++i )
        {
          RC::Handle<Binding> binding = m_postDescendBindings->get(i);
          std::vector<std::string> errors;
          task->m_evaluateParallelCallsPerOperator[ i ] = binding->bind( errors, selfScope );
        }
        taskGroup.add(task);
      }
      
      if ( m_selectBinding && selectorType )
      {
        Bindings::const_iterator it = m_bindings.find( m_selectNodeBindingName );
        FABRIC_ASSERT( it != m_bindings.end() );
        RC::Handle<Node> const &node = it->second;
        
        EventHandlerTask* task = new EventHandlerTask( this, &EventHandler::evaluateLocal, node, selectorType );
        void *prefixes[2] = { &task->m_shouldSelect, &task->m_selectedNode.data[0] };
        std::vector<std::string> errors;
        task->m_selectParallelCall = m_selectBinding->bind( errors, bindingsScope, 2, prefixes );
        taskGroup.add(task);
      }
    }
    
    void EventHandler::evaluateLocal( void *userdata )
    {

    }
    
    
    void EventHandler::collectErrorsForScope( Scope const *parentScope, Errors &errors )
    {
      BindingsScope bindingsScope( m_bindings, parentScope );
      SelfScope selfScope( this, &bindingsScope );
      NamedScope bindingNameScope( m_bindingName, this, &bindingsScope );
      Scope *childScope = m_bindingName.length()>0? (Scope *)&bindingNameScope: (Scope *)&bindingsScope;

      size_t numPreDescendBindings = m_preDescendBindings->size();
      for ( size_t i=0; i<numPreDescendBindings; ++i )
      {
        RC::Handle<Binding> binding = m_preDescendBindings->get(i);
        std::string const errorPrefix = "pre-descend operator " + _(i) + ": ";
        std::vector<std::string> preDescendBindingsErrors;
        binding->bind( preDescendBindingsErrors, selfScope );
        for ( size_t i=0; i<preDescendBindingsErrors.size(); ++i )
          errors.push_back( errorPrefix + preDescendBindingsErrors[i] );
      }
      
      for ( size_t i=0; i<m_childEventHandlers.size(); ++i )
      {
        RC::Handle<EventHandler> const &childEventHandler = m_childEventHandlers[i];
        childEventHandler->collectErrorsForScope( childScope, errors );
      }
    
      size_t numPostDescendBindings = m_postDescendBindings->size();
      for ( size_t i=0; i<numPostDescendBindings; ++i )
      {
        RC::Handle<Binding> binding = m_postDescendBindings->get(i);
        std::string const errorPrefix = "post-descend operator " + _(i) + ": ";
        std::vector<std::string> postDescendBindingsErrors;
        binding->bind( postDescendBindingsErrors, selfScope );
        for ( size_t i=0; i<postDescendBindingsErrors.size(); ++i )
          errors.push_back( errorPrefix + postDescendBindingsErrors[i] );
      }
        
      if ( m_selectBinding )
      {
        void *prefixes[2] = { 0, 0 };
        std::string const errorPrefix = "selector: ";
        std::vector<std::string> selectBindingErrors;
        m_selectBinding->bind( selectBindingErrors, bindingsScope, 2, prefixes );
        for ( size_t i=0; i<selectBindingErrors.size(); ++i )
          errors.push_back( errorPrefix + selectBindingErrors[i] );
      }
    }
    
    void EventHandler::collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const
    {
      for ( Util::UnorderedMap< std::string, RC::Handle<Node> >::const_iterator it=m_bindings.begin(); it!=m_bindings.end(); ++it )
      {
        RC::Handle<Node> const &boundNode = it->second;
        boundNode->collectTasks( generation, taskGroupStream );
      }
      
      for ( size_t i=0; i<m_childEventHandlers.size(); ++i )
      {
        RC::Handle<EventHandler> const &childEventHandler = m_childEventHandlers[i];
        childEventHandler->collectTasks( generation, taskGroupStream );
      }
    }
      
    
    void EventHandler::propagateMarkForRecompileImpl( unsigned generation )
    {
      for ( size_t i=0; i<m_childEventHandlers.size(); ++i )
      {
        RC::Handle<EventHandler> const &childEventHandler = m_childEventHandlers[i];
        childEventHandler->markForRecompile( generation );
      }
      
      for ( ParentEventHandlers::const_iterator it=m_parentEventHandlers.begin(); it!=m_parentEventHandlers.end(); ++it )
      {
        EventHandler *parentEventHandler = *it;
        parentEventHandler->markForRecompile( generation );
      }
      
      for ( std::set< Event * >::const_iterator it=m_events.begin(); it!=m_events.end(); ++it )
      {
        Event *event = *it;
        event->markForRecompile( generation );
      }
    }
     
    void EventHandler::propagateMarkForRefreshImpl( unsigned generation )
    {
    }

    void EventHandler::collectErrors()
    {
      Errors &errors = getErrors();

      if ( m_selectBinding )
      {
        Bindings::const_iterator it = m_bindings.find( m_selectNodeBindingName );
        if ( it == m_bindings.end() )
          errors.push_back( "selector node '" + m_selectNodeBindingName + "' is not bound" );
      }
    }
    
    void EventHandler::invalidateRunState()
    {
      delete m_runState;
      m_runState = 0;
    }
    
    void EventHandler::refreshRunState()
    {
      FABRIC_ASSERT( false && "EventHandler runstates should never need refreshing" );
    }
    
    void EventHandler::ensureRunState() const
    {
      if ( !m_runState )
      {
        m_runState = new RunState;
        m_runState->canExecute = getErrors().empty();
        
        if ( m_runState->canExecute )
        {
          for ( Util::UnorderedMap< std::string, RC::Handle<Node> >::const_iterator it=m_bindings.begin(); it!=m_bindings.end(); ++it )
          {
            RC::Handle<Node> const &boundNode = it->second;
            if ( !boundNode->canExecute() )
            {
              m_runState->canExecute = false;
              break;
            }
          }
        }
          
        if ( m_runState->canExecute )
        {
          for ( size_t i=0; i<m_childEventHandlers.size(); ++i )
          {
            RC::Handle<EventHandler> const &childEventHandler = m_childEventHandlers[i];
            if ( !childEventHandler->canExecute() )
            {
              m_runState->canExecute = false;
              break;
            }
          }
        }
      }
    }
    
    bool EventHandler::canExecute() const
    {
      ensureRunState();
      return m_runState->canExecute;
    }

    void EventHandler::setOutOfDate()
    {
    }
      
    void EventHandler::jsonDescChildEventHandlers( JSON::Encoder &resultEncoder ) const
    {
      JSON::ArrayEncoder childEventHandlersArrayEncoder = resultEncoder.makeArray();
      for ( ChildEventHandlers::const_iterator it=m_childEventHandlers.begin(); it!=m_childEventHandlers.end(); ++it )
      {
        RC::Handle<EventHandler> const &childEventHandler = *it;
        childEventHandlersArrayEncoder.makeElement().makeString( childEventHandler->getName() );
      }
    }
      
    void EventHandler::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      Container::jsonDesc( resultEncoder );
    }

    void EventHandler::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Container::jsonDesc( resultObjectEncoder );
      
      {
        JSON::Encoder memberObjectEncoder = resultObjectEncoder.makeMember( "childEventHandlers", 18 );
        jsonDescChildEventHandlers( memberObjectEncoder );
      }
      
      {
        JSON::Encoder memberObjectEncoder = resultObjectEncoder.makeMember( "scopes", 6 );
        jsonDescScopes( memberObjectEncoder );
      }
      
      {
        JSON::Encoder memberObjectEncoder = resultObjectEncoder.makeMember( "scopeName", 9 );
        memberObjectEncoder.makeString( m_bindingName );
      }
      
      {
        JSON::Encoder memberObjectEncoder = resultObjectEncoder.makeMember( "preDescendBindings", 18 );
        m_preDescendBindings->jsonDesc( memberObjectEncoder );
      }
      
      {
        JSON::Encoder memberObjectEncoder = resultObjectEncoder.makeMember( "postDescendBindings", 19 );
        m_preDescendBindings->jsonDesc( memberObjectEncoder );
      }
      
      JSON::Encoder selectorEncoder = resultObjectEncoder.makeMember( "selector", 8 );
      if ( !m_selectBinding && m_selectNodeBindingName.empty() )
        selectorEncoder.makeNull();
      else
      {
        JSON::ObjectEncoder selectorObjectEncoder = selectorEncoder.makeObject();
        
        {
          JSON::Encoder bindingEncoder = selectorObjectEncoder.makeMember( "binding", 7 );
          if ( m_selectBinding )
            m_selectBinding->jsonDesc( bindingEncoder );
          else bindingEncoder.makeNull();
        }
        
        {
          JSON::Encoder targetScopeNameEncoder = selectorObjectEncoder.makeMember( "targetScopeName", 15 );
          if ( !m_selectNodeBindingName.empty() )
            targetScopeNameEncoder.makeString( m_selectNodeBindingName );
          else targetScopeNameEncoder.makeNull();
        }
      }
    }
      
    void EventHandler::jsonDescType( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeString( "EventHandler", 12 );
    }
      
    void EventHandler::jsonDescScopes( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      for ( ExternalScopes::const_iterator it=m_bindings.begin(); it!=m_bindings.end(); ++it )
      {
        std::string const &name = it->first;
        RC::Handle<Node> const &node = it->second;
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( name );
        memberEncoder.makeString( node->getName() );
      }
    }
      
    void EventHandler::jsonDescBindingName( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeString( m_bindingName );
    }

    void EventHandler::jsonRoute( std::vector<JSON::Entity> const &dst, size_t dstOffset, JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      if ( dst.size() - dstOffset == 1 )
      {
        if ( dst[dstOffset].stringIs( "preDescendBindings", 18 ) )
          m_preDescendBindings->jsonRoute( dst, dstOffset+1, cmd, arg, resultArrayEncoder );
        else if ( dst[dstOffset].stringIs( "postDescendBindings", 19 ) )
          m_postDescendBindings->jsonRoute( dst, dstOffset+1, cmd, arg, resultArrayEncoder );
        else throw Exception( "unrecognized command" );
      }
      else NamedObject::jsonRoute( dst, dstOffset, cmd, arg, resultArrayEncoder );
    }
      
    void EventHandler::jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      if ( cmd.stringIs( "appendChildEventHandler", 23 ) )
        jsonExecAppendChildEventHandler( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "removeChildEventHandler", 23 ) )
        jsonExecRemoveChildEventHandler( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "setScope", 8 ) )
        jsonExecAddScope( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "removeScope", 11 ) )
        jsonExecRemoveScope( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "setSelector", 11 ) )
        jsonExecSetSelector( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "setScopeName", 12 ) )
        jsonExecSetBindingName( arg, resultArrayEncoder );
      else Container::jsonExec( cmd, arg, resultArrayEncoder );
    }

    void EventHandler::jsonExecCreate( JSON::Entity const &arg, RC::Handle<Context> const &context, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      Create( arg.stringToStdString(), context );
    }
    
    void EventHandler::jsonExecAppendChildEventHandler( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      appendChildEventHandler( m_context->getEventHandler( arg.stringToStdString() ) );
    }
    
    void EventHandler::jsonExecRemoveChildEventHandler( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      removeChildEventHandler( m_context->getEventHandler( arg.stringToStdString() ) );
    }
    
    void EventHandler::jsonExecAddScope( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      std::string name;
      RC::Handle<Node> node;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "name", 4 ) )
          {
            valueEntity.requireString();
            name = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "node", 4 ) )
          {
            valueEntity.requireString();
            node = m_context->getNode( valueEntity.stringToStdString() );
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( name.empty() )
        throw Exception( "missing 'name'" );
      if ( !node )
        throw Exception( "missing 'node'" );
      
      setScope( name, node );
    }
    
    void EventHandler::jsonExecRemoveScope( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      std::string scopeName = arg.stringToStdString();
      removeScope( scopeName );
    }
    
    void EventHandler::jsonExecSetSelector( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      std::string targetName;
      RC::Handle<Operator> operator_;
      std::vector<std::string> parameterLayout;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "targetName", 10 ) )
          {
            valueEntity.requireString();
            targetName = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "operator", 8 ) )
          {
            valueEntity.requireString();
            std::string operatorName = valueEntity.stringToStdString();
            operator_ = m_context->getOperator( operatorName );
          }
          else if ( keyString.stringIs( "parameterLayout", 15 ) )
          {
            valueEntity.requireArray();
            JSON::ArrayDecoder valueArrayDecoder( valueEntity );
            JSON::Entity elementEntity;
            while ( valueArrayDecoder.getNext( elementEntity ) )
            {
              try
              {
                elementEntity.requireString();
                parameterLayout.push_back( elementEntity.stringToStdString() );
              }
              catch ( Exception e )
              {
                valueArrayDecoder.rethrow( e );
              }
            }
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( targetName.empty() )
        throw Exception( "missing 'targetName'" );
      if ( !operator_ )
        throw Exception( "missing 'operator'" );
      
      RC::Handle<Binding> binding = Binding::Create( m_context );
      binding->setOperator( operator_ );
      binding->setPrototype( parameterLayout );
      
      setSelector( targetName, binding );
    }
    
    void EventHandler::jsonExecSetBindingName( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      setScopeName( arg.stringToStdString() );
    }
  };
};
