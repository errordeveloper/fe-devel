/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MT_TASK_SET_H
#define _FABRIC_MT_TASK_SET_H

#include <Fabric/Base/Config.h>
#include <Fabric/Base/RC/Handle.h>

#include <vector>
#if defined(FABRIC_OS_LINUX)
# include <string>
#endif

namespace Fabric
{
  namespace MT
  {
    class LogCollector;
    class TaskBase;
    
    class TaskGroup
    {
    public:
    
      TaskGroup();
      TaskGroup( TaskGroup const &that );
      ~TaskGroup();
      
      TaskGroup &operator =( TaskGroup const &that );
      
      void clear();
      void add( TaskBase const *task );
      
      void execute( RC::Handle<LogCollector> const &logCollector, void *userdata ) const;
      
      std::string desc() const;
    
    protected:
    
      struct Context
      {
        TaskGroup const *taskGroup;
        void *userdata;
      };
    
      static void ExecuteTask( void *context, size_t index );
      void executeTask( void *userdata, unsigned index ) const;
    
      std::vector< TaskBase const * > m_tasks;
      bool m_mainThreadOnly;
    };
  };
};

#endif //_FABRIC_MT_TASK_SET_H
