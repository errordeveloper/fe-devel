/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/DG/Node.h>
#include <Fabric/Core/DG/Binding.h>
#include <Fabric/Core/DG/Scope.h>
#include <Fabric/Core/DG/Event.h>
#include <Fabric/Core/DG/EventHandler.h>
#include <Fabric/Core/DG/BindingList.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/ExecutionEngine.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/RT/NumericDesc.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/Util/Base64.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace DG
  {
    struct Node::RunState
    {
      bool canEvaluate;
      MT::TaskGroupStream taskGroupStream;
      std::vector< RC::Handle<MT::ParallelCall> > m_evaluateParallelCallsPerOperator;
    };
    
    size_t Node::s_globalColorGeneration = 0;

    RC::Handle<Node> Node::Create( std::string const &name, RC::Handle<Context> const &context )
    {
      RC::Handle<Node> node = new Node( name, context );

      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        node->jsonDesc( jg );
      }
      node->jsonNotifyDelta( json );
      
      return node;
    }
    
    Node::Node( std::string const &name, RC::Handle<Context> const &context )
      : Container( name, context )
      , m_context( context.ptr() )
      , m_dirty( true )
      , m_globalDependencyRank( 0 )
      , m_evaluateLocalTask( this, &Node::evaluateLocal )
      , m_runState( 0 )
      , m_colorGeneration( s_globalColorGeneration )
    {
      m_bindingList = BindingList::Create( context );
      m_bindingList->addOwner( this, "bindings" );
    }
    
    Node::~Node()
    {
      FABRIC_ASSERT( m_eventHandlers.empty() );

      delete m_runState;

      m_bindingList->removeOwner( this );
    }

    bool Node::isDirty() const {
      return m_dirty;
    }

    void Node::setOutOfDate()
    {
      if ( !m_dirty )
      {
        m_dirty = true;
        
        for ( Dependents::const_iterator it = m_dependents.begin(); it != m_dependents.end(); ++it )
        {
          RC::Handle<Node> const &dependentNode = *it;
          dependentNode->setOutOfDate();
        }
      }
    }
    
    void Node::setDependency( RC::Handle<Node> const &dependencyNode, std::string const &dependencyName )
    {
      if ( dependencyName.empty() )
        throw Exception( "dependencyName must be non-empty" );
      if ( dependencyName == "self" )
        throw Exception( "dependencyName cannot be named 'self'" );
      
      if ( !canBeDependency( dependencyNode ) )
        throw Exception( "cannot set dependency because it would create a dependency loop" );
      
      Dependencies::iterator it = m_dependencies.find( dependencyName );
      if ( it != m_dependencies.end() )
      {
        if ( dependencyNode != it->second )
        {
          RC::Handle<Node> oldParentNode = it->second;
          
          Dependents::iterator jt = oldParentNode->m_dependents.find( this );
          FABRIC_ASSERT( jt != oldParentNode->m_dependents.end() );
          oldParentNode->m_dependents.erase( jt );

          dependencyNode->m_dependents.insert( this );
          
          it->second = dependencyNode;
          
          recalculateGlobalDependencyRank();
        }
      }
      else
      {
        m_dependencies.insert( Util::UnorderedMap< std::string, RC::Handle<Node> >::value_type( dependencyName, dependencyNode ) );
        dependencyNode->m_dependents.insert( this );
      
        maybeIncreaseGlobalDependencyRank( dependencyNode->m_globalDependencyRank );
      }
      
      markForRecompile();
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescDependencies( jg );
      }
      jsonNotifyMemberDelta( "dependencies", 12, json );
    }
    
    void Node::removeDependency( std::string const &dependencyName )
    {
      Dependencies::iterator it = m_dependencies.find( dependencyName );
      if ( it == m_dependencies.end() )
        throw Exception( "no dependency named " + _(dependencyName) );
      
      RC::Handle<Node> dependencyNode = it->second;
      Dependents::iterator jt = dependencyNode->m_dependents.find( this );
      FABRIC_ASSERT( jt != dependencyNode->m_dependents.end() );
      dependencyNode->m_dependents.erase( jt );
      m_dependencies.erase( it );
      
      recalculateGlobalDependencyRank();
      
      markForRecompile();
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescDependencies( jg );
      }
      jsonNotifyMemberDelta( "dependencies", 12, json );
    }
    
    void Node::refreshRunState()
    {
      if ( m_runState )
      {
        for ( size_t i=0; i<m_runState->m_evaluateParallelCallsPerOperator.size(); ++i )
        {
          m_runState->m_evaluateParallelCallsPerOperator[i] = 0;
        }
      }
      
      setOutOfDate();
    }

    RC::Handle<Node> Node::getDependency( std::string const &dependencyName ) const
    {
      Util::UnorderedMap< std::string, RC::Handle<Node> >::const_iterator it = m_dependencies.find( dependencyName );
      if ( it == m_dependencies.end() )
        throw Exception( "no dependency named " + _(dependencyName) );
      return it->second;
    }

    Node::Dependencies const &Node::getDependencies() const
    {
      return m_dependencies;
    }

    Node::Dependents const &Node::getDependents() const
    {
      return m_dependents;
    }

    RC::Handle<BindingList> Node::getBindingList() const
    {
      return m_bindingList;
    }
    
    void Node::addScopeList( RC::Handle<BindingList> const &opList )
    {
      if( opList.isNull() )
        throw Exception( "no operator list given" );
      
      if ( m_bindingList )
        m_bindingList->removeOwner( this );
      m_bindingList = opList;
      if ( m_bindingList )
        m_bindingList->addOwner( this, "bindings" );
    }

    void Node::evaluate()
    {
      PrepareForExecution();
      ensureRunState();
      if ( !m_runState->canEvaluate )
        throw Exception( "cannot execute because of errors" );

      RC::Handle<MT::LogCollector> logCollector( m_context->getLogCollector() );
      ExecutionEngine::ContextSetter contextSetter( m_context );
      m_runState->taskGroupStream.execute( logCollector, 0 );
      if ( logCollector )
        logCollector->flush();
    }

    void Node::evaluateAsync(
      void (*finishedCallback)( void * ),
      void *finishedUserdata
      )
    {
      PrepareForExecution();
      ensureRunState();
      if ( !m_runState->canEvaluate )
        throw Exception( "cannot execute because of errors" );
      
      retain();
      MT::ThreadPool::Instance()->executeParallelAsync(
        m_context->getLogCollector(),
        1,
        &Node::EvaluateAsyncCallback,
        this,
        MT::ThreadPool::Idle,
        finishedCallback,
        finishedUserdata
        );
    }
    
    void Node::EvaluateAsyncCallback(
      void *userdata,
      size_t index
      )
    {
      Node *node = static_cast<Node *>( userdata );
      node->m_context->acquireMutex();
      
      RC::Handle<Context> context = node->m_context;
      RC::Handle<MT::LogCollector> logCollector( context->getLogCollector() );
      ExecutionEngine::ContextSetter contextSetter( context );
      node->m_runState->taskGroupStream.execute( logCollector, 0 );
      if ( logCollector )
        logCollector->flush();
      
      node->m_context->releaseMutex();
      node->release();
    }
    
    void Node::evaluateLocal( void *userdata )
    {
      if ( m_dirty )
      {
        ExecutionEngine::ContextSetter contextSetter( m_context );
        size_t numBindings = m_bindingList->size();
        for ( size_t i=0; i<numBindings; ++i )
        {
          RC::ConstHandle<Binding> binding = m_bindingList->get(i);
          
          RC::Handle<MT::ParallelCall> opParallelCall = m_runState->m_evaluateParallelCallsPerOperator[i];
          if ( !opParallelCall )
          {
            BindingsScope dependenciesScope( m_dependencies );
            SelfScope selfScope( this, &dependenciesScope );
            std::vector<std::string> errors;
            m_runState->m_evaluateParallelCallsPerOperator[i] = opParallelCall = binding->bind( errors, selfScope );
            FABRIC_ASSERT( errors.empty() );
          }

          size_t oldCount = size();
          opParallelCall->executeParallel( m_context->getLogCollector(), m_context, binding->getMainThreadOnly() );
          if( oldCount != size() )
          {
            for ( size_t j=0; j<numBindings; ++j )
              m_runState->m_evaluateParallelCallsPerOperator[j] = 0;
          }
        }
        
        m_dirty = false;
      }
    }
      
    RC::Handle<MT::ParallelCall> Node::bind(
      std::vector<std::string> &errors,
      RC::ConstHandle<Binding> const &binding,
      Scope const &scope,
      unsigned prefixCount,
      void * const *prefixes
      )
    {
      BindingsScope dependenciesScope( m_dependencies, &scope );
      return Container::bind( errors, binding, dependenciesScope, prefixCount, prefixes );
    }
    
    void Node::recalculateGlobalDependencyRank()
    {
      unsigned oldGlobalDependencyRank = m_globalDependencyRank;
      m_globalDependencyRank = 0;
      for ( Dependencies::const_iterator jt=m_dependencies.begin(); jt!=m_dependencies.end(); ++jt )
        if ( jt->second->m_globalDependencyRank >= m_globalDependencyRank )
          m_globalDependencyRank = jt->second->m_globalDependencyRank + 1;
          
      if ( m_globalDependencyRank < oldGlobalDependencyRank )
      {
        for ( Dependents::iterator jt=m_dependents.begin(); jt!=m_dependents.end(); ++jt )
        {
          RC::Handle<Node> const &dependent = *jt;
          dependent->recalculateGlobalDependencyRank();
        }
      }
      else if ( m_globalDependencyRank > oldGlobalDependencyRank )
      {
        for ( Dependents::iterator jt=m_dependents.begin(); jt!=m_dependents.end(); ++jt )
        {
          RC::Handle<Node> const &dependent = *jt;
          dependent->maybeIncreaseGlobalDependencyRank( m_globalDependencyRank );
        }
      }
    }
    
    void Node::maybeIncreaseGlobalDependencyRank( size_t dependencyGlobalDependencyRank )
    {
      if ( dependencyGlobalDependencyRank >= m_globalDependencyRank )
      {
        m_globalDependencyRank = dependencyGlobalDependencyRank + 1;
        for ( Dependents::const_iterator it=m_dependents.begin(); it!=m_dependents.end(); ++it )
        {
          RC::Handle<Node> const &dependent = *it;
          dependent->maybeIncreaseGlobalDependencyRank( m_globalDependencyRank );
        }
      }
    }
    
    void Node::propagateMarkForRecompileImpl( unsigned generation )
    {
      for( Dependencies::const_iterator it=m_dependencies.begin(); it!=m_dependencies.end(); ++it )
      {
        RC::Handle<Node> const &dependency = it->second;
        dependency->markForRecompile( generation );
      }
      
      for( Dependents::const_iterator it=m_dependents.begin(); it!=m_dependents.end(); ++it )
      {
        RC::Handle<Node> const &dependent = *it;
        dependent->markForRecompile( generation );
      }
      
      for ( EventHandlers::const_iterator it=m_eventHandlers.begin(); it!=m_eventHandlers.end(); ++it )
      {
        EventHandler *eventHandler = *it;
        eventHandler->markForRecompile( generation );
      }
    }
    
    void Node::propagateMarkForRefreshImpl( unsigned generation )
    {
      // [pzion 20110303] Note that this is too much; we only want
      // to refresh our direct dependents becaue they are the only 
      // compiled objects which can currently have handles to our
      // member data
      for( Dependents::const_iterator it=m_dependents.begin(); it!=m_dependents.end(); ++it )
      {
        RC::Handle<Node> const &dependent = *it;
        dependent->markForRefresh( generation );
      }
    }
       
    void Node::collectErrors()
    {
      Errors &errors = getErrors();
      BindingsScope dependenciesScope( m_dependencies );
      SelfScope selfScope( this, &dependenciesScope );

      size_t numBindings = m_bindingList->size();
      for ( size_t i=0; i<numBindings; ++i )
      {
        RC::ConstHandle<Binding> binding = m_bindingList->get(i);
        std::string const errorPrefix = "binding " + _(i) + ": ";
        std::vector<std::string> bindErrors;
        RC::Handle<MT::ParallelCall>( binding->bind( bindErrors, selfScope ) );
        for ( size_t i=0; i<bindErrors.size(); ++i )
          errors.push_back( errorPrefix + bindErrors[i] );
      }
    }

    bool Node::canExecute() const
    {
      ensureRunState();
      return m_runState->canEvaluate;
    }
    
    void Node::ensureRunState() const
    {
      if ( !m_runState )
      {
        m_runState = new RunState;
        
        m_runState->canEvaluate = getErrors().empty();
        if ( m_runState->canEvaluate )
        {
          for( Dependencies::const_iterator it=m_dependencies.begin(); it!=m_dependencies.end(); ++it )
          {
            RC::Handle<Node> const &dependency = it->second;
            if ( !dependency->canExecute() )
            {
              m_runState->canEvaluate = false;
              break;
            }
          }
        }
          
        if ( m_runState->canEvaluate )
        {
          collectTasks( m_runState->taskGroupStream );
          m_runState->m_evaluateParallelCallsPerOperator.resize( m_bindingList->size(), 0 );
        }
      }
    }
       
    void Node::invalidateRunState()
    {
      if ( m_runState )
      {
        delete m_runState;
        m_runState = 0;
      }
      
      setOutOfDate();
    }

    void Node::collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const
    {
      taskGroupStream.add( m_globalDependencyRank, &m_evaluateLocalTask );
      
      for ( Util::UnorderedMap< std::string, RC::Handle<Node> >::const_iterator it=m_dependencies.begin(); it!=m_dependencies.end(); ++it )
      {
        RC::Handle<Node> const &dependency = it->second;
        dependency->collectTasks( generation, taskGroupStream );
      }
    }
      
    bool Node::canBeDependency( RC::Handle<Node> const &node ) const
    {
      size_t colorGeneration = ++s_globalColorGeneration;
      FABRIC_ASSERT( m_colorGeneration != colorGeneration );
      node->colorSelfAndDependencies( colorGeneration );
      return m_colorGeneration != colorGeneration;
    }
    
    void Node::colorSelfAndDependencies( size_t colorGeneration )
    {
      if ( m_colorGeneration != colorGeneration )
      {
        m_colorGeneration = colorGeneration;
        for ( Dependencies::const_iterator it=m_dependencies.begin(); it!=m_dependencies.end(); ++it )
        {
          RC::Handle<Node> const &dependencyNode = it->second;
          dependencyNode->colorSelfAndDependencies( colorGeneration );
        }
      }
    }

    void Node::addEventHandler( EventHandler *eventHandler )
    {
      m_eventHandlers.insert( eventHandler );
    }

    void Node::removeEventHandler( EventHandler *eventHandler )
    {
      EventHandlers::iterator it = m_eventHandlers.find( eventHandler );
      FABRIC_ASSERT( it != m_eventHandlers.end() );
      m_eventHandlers.erase( it );
    }

    RC::Handle<Context> Node::getContext() const
    {
      return m_context;
    }

    void Node::jsonRoute(
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( dst.size() - dstOffset == 1 && dst[dstOffset].stringIs( "bindings", 8 ) )
      {
        try
        {
          m_bindingList->jsonRoute( dst, dstOffset+1, cmd, arg, resultArrayEncoder );
        }
        catch ( Exception e )
        {
          throw "bindings: " + e;
        }
      }
      else NamedObject::jsonRoute( dst, dstOffset, cmd, arg, resultArrayEncoder );
     }

    void Node::jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      if ( cmd.stringIs( "setDependency", 13 ) )
        jsonExecAddDependency( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "removeDependency", 16 ) )
        jsonExecRemoveDependency( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "evaluate", 8 ) )
        jsonExecEvaluate( resultArrayEncoder );
      else if ( cmd.stringIs( "evaluateAsync", 13 ) )
        jsonExecEvaluateAsync( arg, resultArrayEncoder );
      else Container::jsonExec( cmd, arg, resultArrayEncoder );
    }
    
    void Node::jsonExecCreate( JSON::Entity const &arg, RC::Handle<Context> const &context, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      Create( arg.stringToStdString(), context );
    }
      
    void Node::jsonExecAddDependency(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
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
            std::string nodeName = valueEntity.stringToStdString();
            node = m_context->getNode( nodeName );
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
      
      setDependency( node, name );
    }
    
    void Node::jsonExecRemoveDependency( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      std::string dependencyName = arg.stringToStdString();
      removeDependency( dependencyName );
    }
    
    void Node::jsonExecEvaluate( JSON::ArrayEncoder &resultArrayEncoder )
    {
      evaluate();
    }
    
    void Node::jsonExecEvaluateAsync( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireInteger();
      int32_t serial = arg.integerValue();
      
      JSONEvaluateAsyncUserdata *jsonEvaluateAsyncUserdata = new JSONEvaluateAsyncUserdata;
      jsonEvaluateAsyncUserdata->node = this;
      {
        JSON::Encoder jg( &jsonEvaluateAsyncUserdata->notifyJSONArg );
        jg.makeInteger( serial );
      }
        
      evaluateAsync( &Node::JSONExecEvaluateAsyncFinishedCallback, jsonEvaluateAsyncUserdata );
    }
    
    void Node::JSONExecEvaluateAsyncFinishedCallback( void *userdata )
    {
      JSONEvaluateAsyncUserdata *jsonEvaluateAsyncUserdata = static_cast<JSONEvaluateAsyncUserdata *>( userdata );
      
      jsonEvaluateAsyncUserdata->node->jsonNotify( "evaluateAsyncFinished", 21, &jsonEvaluateAsyncUserdata->notifyJSONArg );
      
      delete jsonEvaluateAsyncUserdata;
    }
    
    void Node::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      Container::jsonDesc( resultEncoder );
    }
    
    void Node::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Container::jsonDesc( resultObjectEncoder );
      
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "dependencies", 12 );
        jsonDescDependencies( memberEncoder );
      }

      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "bindings", 8 );
        m_bindingList->jsonDesc( memberEncoder );
      }
    }
    
    void Node::jsonDescType( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeString( "Node", 4 );
    }
      
    void Node::jsonDescDependencies( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      for ( Dependencies::const_iterator it=m_dependencies.begin(); it!=m_dependencies.end(); ++it )
      {
        std::string const &name = it->first;
        RC::Handle<Node> const &node = it->second;
        resultObjectEncoder.makeMember( name ).makeString( node->getName() );
      }
    }
  };
};
