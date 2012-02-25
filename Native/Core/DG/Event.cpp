/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Event.h"
#include "EventHandler.h"
#include "Node.h"
#include "Operator.h"
#include "Scope.h"
#include "SelectedNode.h"
#include "Debug.h"
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/ExecutionEngine.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/MT/LogCollector.h>

namespace Fabric
{
  

  namespace DG
  {
    
    EventTaskGroup::~EventTaskGroup()
    {
      clear();
    }
    
    void EventTaskGroup::clear()
    {
      for ( size_t i=0; i<m_tasks.size(); ++i )
      {
        delete (EventHandlerTask*)m_tasks[i];
      }
      m_tasks.resize(0);
    }
    
    void EventTaskGroup::execute( RC::Handle<MT::LogCollector> const &logCollector, void *userdata ) const
    {
      if ( !m_tasks.empty() )
      {
        for ( size_t i=0; i<m_tasks.size(); ++i )
        {
          m_tasks[i]->execute(userdata);
        }
      }
    }
    
    
    RC::Handle<Event> Event::Create( std::string const &name, RC::Handle<Context> const &context )
    {
      RC::Handle<Event> event = new Event( name, context );
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        event->jsonDesc( jg );
      }
      event->jsonNotifyDelta( json );

      return event;
    }
    
    Event::Event( std::string const &name, RC::Handle<Context> const &context )
      : Container( name, context )
      , m_context( context.ptr() )
      , m_runState( 0 )
    {
    }
    
    Event::~Event()
    {
      for ( size_t i=0; i<m_eventHandlers.size(); ++i )
      {
        RC::Handle<EventHandler> const &eventHandler = m_eventHandlers[i];
        eventHandler->removeEvent( this );
      }

      delete m_runState;
    }
      
    void Event::setOutOfDate()
    {
    }
      
    void Event::appendEventHandler( RC::Handle<EventHandler> const &eventHandler )
    {
      if ( !eventHandler )
        throw Exception( "event handler is null" );
      
      m_eventHandlers.push_back( eventHandler );
      eventHandler->addEvent( this );
      
      markForRecompile();
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescEventHandlers( jg );
      }
      jsonNotifyMemberDelta( "eventHandlers", 13, json );
    }
    
    Event::EventHandlers const &Event::getEventHandlers() const
    {
      return m_eventHandlers;
    }
      
    void Event::fire() const
    {
      PrepareForExecution( m_context );
      fire( 0, 0 );
    }
    
    void Event::setSelectType( RC::ConstHandle<RT::Desc> const &selectorType )
    {
      m_selectorType = selectorType;
      markForRecompile();
    }
    
    void Event::select( SelectedNodeList &selectedNodes ) const
    {
      if(!m_selectorType){
        throw Exception( "select type not defined" );
      }
      PrepareForExecution( m_context );
      fire( 0, &selectedNodes );
    }
    
    void Event::fire( Scope const *parentScope, SelectedNodeList *selectedNodes ) const
    {
      ensureRunState();
      if ( !m_runState->canExecute )
        throw Exception( "cannot execute because of errors" );
 
      ExecutionEngine::ContextSetter contextSetter( m_context );
      m_runState->taskGroupStream.execute( m_context->getLogCollector(), 0 );
      m_runState->taskGroup.execute( m_context->getLogCollector(), selectedNodes );
        
      if ( m_context->getLogCollector() )
        m_context->getLogCollector()->flush();
    }
    
      
    void Event::collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const
    {
      for ( EventHandlers::const_iterator it=m_eventHandlers.begin(); it!=m_eventHandlers.end(); ++it )
      {
        RC::Handle<EventHandler> const &eventHandler = *it;
        eventHandler->collectTasks( generation, taskGroupStream );
      }
    }
    
    
    void Event::collectEventTasksImpl( EventTaskGroup &taskGroup ) const
    {
      for ( EventHandlers::const_iterator it=m_eventHandlers.begin(); it!=m_eventHandlers.end(); ++it )
      {
        RC::Handle<EventHandler> const &eventHandler = *it;
        eventHandler->collectEventTasksImpl( taskGroup, m_selectorType, 0 );
      }
    }
    
    void Event::propagateMarkForRecompileImpl( unsigned generation )
    {
      for ( EventHandlers::const_iterator it=m_eventHandlers.begin(); it!=m_eventHandlers.end(); ++it )
      {
        RC::Handle<EventHandler> const &eventHandler = *it;
        eventHandler->markForRecompile( generation );
      }
    }
    
    void Event::propagateMarkForRefreshImpl( unsigned generation )
    {
    }
    
    void Event::collectErrors()
    {
      collectErrors( 0 );
    }
    
    void Event::collectErrors( Scope *scope )
    {
      Errors &errors = getErrors();
      for ( EventHandlers::const_iterator it=m_eventHandlers.begin(); it!=m_eventHandlers.end(); ++it )
      {
        RC::Handle<EventHandler> const &eventHandler = *it;
        eventHandler->collectErrorsForScope( scope, errors );
      }
    }
    
    void Event::invalidateRunState()
    {
      delete m_runState;
      m_runState = 0;
    }
    
    void Event::refreshRunState()
    {
      FABRIC_ASSERT( false && "Event runstates should never be refreshed" );
    }
    
    void Event::ensureRunState() const
    {
      if ( !m_runState )
      {
        m_runState = new RunState;
        
        m_runState->canExecute = getErrors().empty();
        if ( m_runState->canExecute )
        {
          for ( EventHandlers::const_iterator it=m_eventHandlers.begin(); it!=m_eventHandlers.end(); ++it )
          {
            RC::Handle<EventHandler> const &eventHandler = *it;
            if ( !eventHandler->canExecute() )
            {
              m_runState->canExecute = false;
              break;
            }
          }
        }

        if ( m_runState->canExecute ){
          collectTasks( m_runState->taskGroupStream );
          
          // Now collect the tasks for the entire event tree.
          m_runState->taskGroup.clear();
          collectEventTasksImpl( m_runState->taskGroup );
        }
      }
    }

    bool Event::canExecute() const
    {
      ensureRunState();
      return m_runState->canExecute;
    }
      
    void Event::jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      if ( cmd.stringIs( "appendEventHandler", 18 ) )
        jsonExecAppendEventHandler( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "fire", 4 ) )
        jsonExecFire( resultArrayEncoder );
      else if ( cmd.stringIs( "setSelectType", 13 ) )
        jsonExecSetSelectType( arg );
      else if ( cmd.stringIs( "select", 6 ) )
        jsonExecSelect( resultArrayEncoder );
      else Container::jsonExec( cmd, arg, resultArrayEncoder );
    }

    void Event::jsonExecCreate( JSON::Entity const &arg, RC::Handle<Context> const &context, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      Create( arg.stringToStdString(), context );
    }
    
    void Event::jsonExecAppendEventHandler( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      appendEventHandler( m_context->getEventHandler( arg.stringToStdString() ) );
    }
    
    void Event::jsonExecFire( JSON::ArrayEncoder &resultArrayEncoder )
    {
      fire();
    }
    
    
    void Event::jsonExecSetSelectType( JSON::Entity const &arg )
    {
      arg.requireString();
      std::string selectorTypeName = arg.stringToStdString();
      setSelectType( m_context->getRTManager()->getDesc( selectorTypeName ) );
    }
    
    void Event::jsonExecSelect( JSON::ArrayEncoder &resultArrayEncoder )
    {
      SelectedNodeList selectedNodeList;
      select( selectedNodeList );
      
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      JSON::ArrayEncoder elementsArrayEncoder = resultEncoder.makeArray();
      for ( SelectedNodeList::const_iterator it=selectedNodeList.begin(); it!=selectedNodeList.end(); ++it )
      {
        SelectedNode const &selectedNode = *it;
        JSON::Encoder elementEncoder = elementsArrayEncoder.makeElement();
        JSON::ObjectEncoder elementObjectEncoder = elementEncoder.makeObject();
        {
          elementObjectEncoder.makeMember( "node", 4 ).makeString( selectedNode.node->getName() );
        }
        {
          JSON::Encoder dataEncoder = elementObjectEncoder.makeMember( "data", 4 );
          m_selectorType->encodeJSON( &selectedNode.data[0], dataEncoder );
        }
      }
    }
      
    void Event::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      Container::jsonDesc( resultEncoder );
    }
    
    void Event::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Container::jsonDesc( resultObjectEncoder );
      JSON::Encoder eventHandlersEncoder = resultObjectEncoder.makeMember( "eventHandlers", 13 );
      jsonDescEventHandlers( eventHandlersEncoder );
    }
      
    void Event::jsonDescType( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeString( "Event", 5 );
    }
      
    void Event::jsonDescEventHandlers( JSON::Encoder &resultEncoder ) const
    {
      JSON::ArrayEncoder eventHandlersArrayEncoder = resultEncoder.makeArray();
      for ( EventHandlers::const_iterator it=m_eventHandlers.begin(); it!=m_eventHandlers.end(); ++it )
      {
        RC::Handle<EventHandler> const &eventHandler = *it;
        eventHandlersArrayEncoder.makeElement().makeString( eventHandler->getName() );
      }
    }
  };
};
