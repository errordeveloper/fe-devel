/*
 *
 *  Created by Peter Zion on 10-09-16.
 *  Copyright 2010 Fabric 3D Inc. All rights reserved.
 *
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
    
      virtual bool isEvent() const { return true; }

      typedef std::vector< RC::Handle<EventHandler> > EventHandlers;
      
      static RC::Handle<Event> Create( std::string const &name, RC::Handle<Context> const &context );
      
      void appendEventHandler( RC::Handle<EventHandler> const &eventHandler );
      EventHandlers const &getEventHandlers() const;
      
      void fire() const;
      void setSelectType( RC::ConstHandle<RT::Desc> const &selectorType );
      void select( SelectedNodeList &selectedNodes ) const;
      
      virtual void jsonExec( std::string const &cmd, RC::ConstHandle<JSON::Value> const &arg, Util::JSONArrayGenerator &resultJAG );
      static void jsonExecCreate( RC::ConstHandle<JSON::Value> const &arg, RC::Handle<Context> const &context, Util::JSONArrayGenerator &resultJAG );
      void jsonExecAppendEventHandler( RC::ConstHandle<JSON::Value> const &arg, Util::JSONArrayGenerator &resultJAG );
      void jsonExecFire( Util::JSONArrayGenerator &resultJAG );
      void jsonExecSetSelectType( RC::ConstHandle<JSON::Value> const &arg );
      void jsonExecSelect( Util::JSONArrayGenerator &resultJAG );
      void jsonDesc( Util::JSONGenerator &resultJG ) const;
      virtual void jsonDesc( Util::JSONObjectGenerator &resultJOG ) const;
      virtual void jsonDescType( Util::JSONGenerator &resultJG ) const;
      void jsonDescEventHandlers( Util::JSONGenerator &resultJG ) const;
      
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
