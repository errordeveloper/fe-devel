/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_COMPILED_OBJECT_H
#define _FABRIC_DG_COMPILED_OBJECT_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/Util/Mutex.h>

#include <set>
#include <vector>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  }
  
  namespace MT
  {
    class TaskGroupStream;
  }
  
  namespace DG
  {
    class Context;
    
    class CompiledObject : public RC::Object
    {
    public:
    
      struct GlobalData
      {
        std::set< RC::Handle<CompiledObject> > compiledObjectsMarkedForRecompile;
        unsigned markForRecompileGlobalGeneration;
        std::set< RC::Handle<CompiledObject> > compiledObjectsMarkedForRefresh;
        Util::Mutex markForRefreshMutex;
        unsigned markForRefreshGlobalGeneration;
        unsigned collectTasksGlobalGeneration;
        
        GlobalData()
          : markForRecompileGlobalGeneration( 0 )
          , markForRefreshMutex( "DG::CompiledObject::MarkForRefresh" )
          , markForRefreshGlobalGeneration( 0 )
          , collectTasksGlobalGeneration( 0 )
        {
        }
      };
    
      REPORT_RC_LEAKS

      typedef std::vector<std::string> Errors;
    
      virtual bool isNamedObject() const { return false; }
      
      Errors const &getErrors() const;
      Errors &getErrors();
      virtual bool canExecute() const = 0;
      
      void markForRecompile( unsigned generation );
      void markForRefresh( unsigned generation );
      
      static void PrepareForExecution( RC::Handle<Context> const &context );
      
      virtual void jsonNotifyErrorDelta() const;
      void jsonDescErrors( JSON::Encoder &resultEncoder ) const;

    protected:
    
      CompiledObject( RC::Handle<Context> const &context );
      
      void markForRecompile();
      virtual void propagateMarkForRecompileImpl( unsigned generation ) = 0;    

      static void Compile( std::set< RC::Handle<CompiledObject> > const &compiledObjectsMarkedForRecompile );
      
      virtual void collectErrors() = 0;
      virtual void invalidateRunState() = 0;
      
      void markForRefresh();
      virtual void propagateMarkForRefreshImpl( unsigned generation ) = 0;    

      static void Refresh( std::set< RC::Handle<CompiledObject> > const &compiledObjectsMarkedForRefresh );

      virtual void refreshRunState() = 0;

      void collectTasks( MT::TaskGroupStream &taskGroupStream ) const;
      void collectTasks( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const;
      virtual void collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const = 0;

    private:
    
      Context *m_context;
    
      mutable unsigned m_markForRecompileGeneration;
      mutable unsigned m_markForRefreshGeneration;
      mutable unsigned m_collectTasksGeneration;
      
      Errors m_errors;
      bool m_canExecute;
    };
  }
}

#endif //_FABRIC_DG_COMPILED_OBJECT_H
