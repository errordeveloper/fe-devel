/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_EVENT_HANDLER_H
#define _FABRIC_DG_EVENT_HANDLER_H

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
    class Binding;
    class BindingList;
    class Event;
    class EventTaskGroup;
    
    class EventHandlerTask : public MT::Task<EventHandler> 
    {
    public:
      
      EventHandlerTask( EventHandler *t, MT::Task<EventHandler>::ExecuteCallback executeCallback );
      EventHandlerTask( EventHandler *t, MT::Task<EventHandler>::ExecuteCallback executeCallback, RC::Handle<Node> const &node, RC::ConstHandle<RT::Desc> const &selectorType );
      
      virtual void execute( void *userdata ) const;
      
      std::vector< RC::Handle<MT::ParallelCall> > m_evaluateParallelCallsPerOperator;
      SelectedNode m_selectedNode;
      bool m_shouldSelect;
      RC::Handle<MT::ParallelCall> m_selectParallelCall;
      Context *m_context;
    };
    
    class EventHandler : public Container
    {
      friend class Node;
      friend class Event;
      friend class EventHandlerTask;
      
      typedef Util::UnorderedMap< std::string, RC::Handle<Node> > ExternalScopes;
      
      
    public:
      REPORT_RC_LEAKS
    
      typedef Util::UnorderedMap< std::string, RC::Handle<Node> > Bindings;
      typedef std::vector< RC::Handle<EventHandler> > ChildEventHandlers;
      typedef std::multiset<EventHandler *> ParentEventHandlers;

      static RC::Handle<EventHandler> Create( std::string const &name, RC::Handle<Context> const &context );
      
      virtual bool isEventHandler() const { return true; }

      void appendChildEventHandler( RC::Handle<EventHandler> const &event );
      ChildEventHandlers const &getChildEventHandlers() const;
      void removeChildEventHandler( RC::Handle<EventHandler> const &event );
      
      ParentEventHandlers const &getParentEventHandlers() const;
      
      Bindings const &getScopes() const;
      void setScope( std::string const &name, RC::Handle<Node> const &node );
      void removeScope( std::string const &name );
      std::string const &getScopeName() const;
      void setScopeName( std::string const &bindingName );
      
      RC::Handle<BindingList> getPreDescendBindingList( ) const;
      RC::Handle<BindingList> getPostDescendBindingList( ) const;
      void setPreDescendBindingList( RC::Handle<BindingList> opList );
      void setPostDescendBindingList( RC::Handle<BindingList> opList );
      
      void setSelector( std::string const &targetScopeName, RC::Handle<Binding> const &selector );
      
      virtual void jsonRoute( std::vector<JSON::Entity> const &dst, size_t dstOffset, JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      virtual void jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      static void jsonExecCreate( JSON::Entity const &arg, RC::Handle<Context> const &context, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecAppendChildEventHandler( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecRemoveChildEventHandler( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecAddScope( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecRemoveScope( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecSetSelector( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecSetBindingName( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonDesc( JSON::Encoder &resultEncoder ) const;
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      virtual void jsonDescType( JSON::Encoder &resultEncoder ) const;
      void jsonDescChildEventHandlers( JSON::Encoder &resultEncoder ) const;
      void jsonDescScopes( JSON::Encoder &resultEncoder ) const;
      void jsonDescBindingName( JSON::Encoder &resultEncoder ) const;
      
    protected:
    
      EventHandler( std::string const &name, RC::Handle<Context> const &context );
      ~EventHandler();
      
      virtual void setOutOfDate();
      
      void addEvent( Event *event );
      void removeEvent( Event *event );
      void addParent( EventHandler *eventHandler );
      void removeParent( EventHandler *eventHandler );
      
      void fire( Scope const *parentScope, RC::ConstHandle<RT::Desc> const &selectorType, SelectedNodeList *selectedNodes );
      
      void collectEventTasksImpl( EventTaskGroup &taskGroup, RC::ConstHandle<RT::Desc> const &selectorType, Scope const *parentScope );
      
      void collectErrorsForScope( Scope const *parentScope, Errors &errors );
    
      virtual void propagateMarkForRecompileImpl( unsigned generation );    
      virtual void propagateMarkForRefreshImpl( unsigned generation );    
      virtual void collectErrors();
      virtual void invalidateRunState();
      virtual void refreshRunState();
      virtual void collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const;
      virtual bool canExecute() const;
      
      virtual void evaluateLocal( void *userdata );
      
      void ensureRunState() const;
      
    private:
    
      Context *m_context;
      
      std::set<Event *> m_events;

      ParentEventHandlers m_parentEventHandlers;
      ChildEventHandlers m_childEventHandlers;

      ExternalScopes m_bindings;
      std::string m_bindingName;
      
      RC::Handle<BindingList> m_preDescendBindings;
      RC::Handle<BindingList> m_postDescendBindings;

      std::string m_selectNodeBindingName;
      RC::Handle<Binding> m_selectBinding;
      
      struct RunState
      {
        bool canExecute;
      };
      
      mutable RunState *m_runState;
    };
  };
};

#endif //_FABRIC_DG_EVENT_HANDLER_H
