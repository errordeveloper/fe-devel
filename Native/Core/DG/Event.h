/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_EVENT_H
#define _FABRIC_DG_EVENT_H

#include <Fabric/Core/DG/Container.h>
#include <Fabric/Core/DG/SelectedNode.h>
#include <list>
#include <set>

namespace Fabric
{
  namespace DG
  {
    class Context;
    class Node;
    class Scope;
    class Operator;
    class EventHandler;
    
    
    class EventTaskGroup : public MT::TaskGroup
    {
    public:
      
      ~EventTaskGroup();
      
      void clear();
      
      void execute( RC::Handle<MT::LogCollector> const &logCollector, void *userdata ) const;
      
    private:
      
      
    };
    
    class Event : public Container
    {
      friend class EventHandler;
      
    public:
      REPORT_RC_LEAKS
    
      virtual bool isEvent() const { return true; }

      typedef std::vector< RC::Handle<EventHandler> > EventHandlers;
      
      static RC::Handle<Event> Create( std::string const &name, RC::Handle<Context> const &context );
      
      void appendEventHandler( RC::Handle<EventHandler> const &eventHandler );
      EventHandlers const &getEventHandlers() const;
      
      void fire() const;
      void setSelectType( RC::ConstHandle<RT::Desc> const &selectorType );
      void select( SelectedNodeList &selectedNodes ) const;
      
      virtual void jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      static void jsonExecCreate( JSON::Entity const &arg, RC::Handle<Context> const &context, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecAppendEventHandler( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecFire( JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecSetSelectType( JSON::Entity const &arg );
      void jsonExecSelect( JSON::ArrayEncoder &resultArrayEncoder );
      void jsonDesc( JSON::Encoder &resultEncoder ) const;
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      virtual void jsonDescType( JSON::Encoder &resultEncoder ) const;
      void jsonDescEventHandlers( JSON::Encoder &resultEncoder ) const;
      
    protected:
    
      Event( std::string const &name, RC::Handle<Context> const &context );
      ~Event();
      
      virtual void setOutOfDate();
    
      virtual void propagateMarkForRecompileImpl( unsigned generation );    
      virtual void propagateMarkForRefreshImpl( unsigned generation );    
      virtual void collectErrors();
      virtual void invalidateRunState();
      virtual void refreshRunState();
      virtual void collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const;
      
      virtual void collectEventTasksImpl( EventTaskGroup &taskGroup ) const;
      
      virtual bool canExecute() const;
      
      void ensureRunState() const;
      
      void fire( Scope const *parentScope, SelectedNodeList *selectedNodes ) const;

      void collectErrors( Scope *scope );

    private:
    
      Context *m_context;
    
      EventHandlers m_eventHandlers;
      RC::ConstHandle<RT::Desc> m_selectorType;
      
      struct RunState
      {
        bool canExecute;
        MT::TaskGroupStream taskGroupStream;
        EventTaskGroup taskGroup;
      };

      mutable RunState *m_runState;
    };
  };
};

#endif //_FABRIC_DG_EVENT_H
