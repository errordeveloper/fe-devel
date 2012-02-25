/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/CompiledObject.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Base/JSON/Encoder.h>

#include <map>

namespace Fabric
{
  namespace DG
  {
    CompiledObject::CompiledObject( RC::Handle<Context> const &context )
      : m_context( context.ptr() )
      , m_canExecute( true )
    {
      GlobalData *cogd = m_context->getCompiledObjectGlobalData();
      m_markForRecompileGeneration = cogd->markForRecompileGlobalGeneration;
      m_markForRefreshGeneration = cogd->markForRefreshGlobalGeneration;
      m_collectTasksGeneration = cogd->collectTasksGlobalGeneration;
    }
    
    CompiledObject::Errors const &CompiledObject::getErrors() const
    {
      return m_errors;
    }
    
    CompiledObject::Errors &CompiledObject::getErrors()
    {
      return m_errors;
    }
    
    bool CompiledObject::canExecute() const
    {
      return m_canExecute;
    }
    
    void CompiledObject::markForRecompile()
    {
      GlobalData *cogd = m_context->getCompiledObjectGlobalData();
      markForRecompile( cogd->markForRecompileGlobalGeneration + 1 );
    }
    
    void CompiledObject::markForRecompile( unsigned generation )
    {
      if ( m_markForRecompileGeneration != generation )
      {
        GlobalData *cogd = m_context->getCompiledObjectGlobalData();
        cogd->compiledObjectsMarkedForRecompile.insert( this );
        m_markForRecompileGeneration = generation;
        propagateMarkForRecompileImpl( generation );
      }
    }
    
    void CompiledObject::Compile( std::set< RC::Handle<CompiledObject> > const &taskCollectors )
    {
      std::map< RC::Handle<CompiledObject>, bool > hadErrorsMap;
      
      for ( std::set< RC::Handle<CompiledObject> >::const_iterator it=taskCollectors.begin(); it!=taskCollectors.end(); ++it )
      {
        RC::Handle<CompiledObject> const &compiledObject = *it;
        hadErrorsMap[compiledObject] = !compiledObject->m_errors.empty();
        compiledObject->m_errors.clear();
      }

      for ( std::set< RC::Handle<CompiledObject> >::const_iterator it=taskCollectors.begin(); it!=taskCollectors.end(); ++it )
      {
        RC::Handle<CompiledObject> const &compiledObject = *it;
        compiledObject->collectErrors();
      }

      for ( std::set< RC::Handle<CompiledObject> >::const_iterator it=taskCollectors.begin(); it!=taskCollectors.end(); ++it )
      {
        RC::Handle<CompiledObject> const &compiledObject = *it;
        if ( hadErrorsMap[compiledObject] || !compiledObject->m_errors.empty() )
          compiledObject->jsonNotifyErrorDelta();
      }

      for ( std::set< RC::Handle<CompiledObject> >::const_iterator it=taskCollectors.begin(); it!=taskCollectors.end(); ++it )
      {
        RC::Handle<CompiledObject> const &taskCollector = *it;
        taskCollector->invalidateRunState();
      }
    }
    
    void CompiledObject::markForRefresh()
    {
      GlobalData *cogd = m_context->getCompiledObjectGlobalData();
      markForRefresh( cogd->markForRefreshGlobalGeneration + 1 );
    }
    
    void CompiledObject::markForRefresh( unsigned generation )
    {
      if ( m_markForRefreshGeneration != generation )
      {
        GlobalData *cogd = m_context->getCompiledObjectGlobalData();
        {//Scope mutexLock
          //[JCG 20111220 Container::resize can happen in parallel and will call this function]
          Util::Mutex::Lock mutexLock( cogd->markForRefreshMutex );
          cogd->compiledObjectsMarkedForRefresh.insert( this );
        }
        m_markForRefreshGeneration = generation;
        propagateMarkForRefreshImpl( generation );
      }
    }
    
    void CompiledObject::Refresh( std::set< RC::Handle<CompiledObject> > const &taskCollectors )
    {
      for ( std::set< RC::Handle<CompiledObject> >::const_iterator it=taskCollectors.begin(); it!=taskCollectors.end(); ++it )
      {
        RC::Handle<CompiledObject> const &taskCollector = *it;
        taskCollector->refreshRunState();
      }
    }
    
    void CompiledObject::collectTasks( MT::TaskGroupStream &taskGroupStream ) const
    {
      GlobalData *cogd = m_context->getCompiledObjectGlobalData();
      collectTasks( ++cogd->collectTasksGlobalGeneration, taskGroupStream );
    }
      
    void CompiledObject::collectTasks( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const
    {
      if ( m_collectTasksGeneration != generation )
      {
        m_collectTasksGeneration = generation;
        collectTasksImpl( generation, taskGroupStream );
      }
    }
      
    void CompiledObject::jsonDescErrors( JSON::Encoder &resultEncoder ) const
    {
      Errors const &errors = getErrors();

      JSON::ArrayEncoder resultArrayEncoder = resultEncoder.makeArray();
      for ( size_t i=0; i<errors.size(); ++i )
      {
        JSON::Encoder errorEncoder = resultArrayEncoder.makeElement();
        errorEncoder.makeString( errors[i] );
      }
    }
    
    void CompiledObject::jsonNotifyErrorDelta() const
    {
    }
    
    void CompiledObject::PrepareForExecution( RC::Handle<Context> const &context )
    {
      GlobalData *cogd = context->getCompiledObjectGlobalData();

      if ( !cogd->compiledObjectsMarkedForRecompile.empty() )
      {
        Compile( cogd->compiledObjectsMarkedForRecompile );
        cogd->compiledObjectsMarkedForRecompile.clear();
      }
      ++cogd->markForRecompileGlobalGeneration;
      
      if ( !cogd->compiledObjectsMarkedForRefresh.empty() )
      {
        Refresh( cogd->compiledObjectsMarkedForRefresh );
        cogd->compiledObjectsMarkedForRefresh.clear();
      }
      ++cogd->markForRefreshGlobalGeneration;
    }
  }
}
