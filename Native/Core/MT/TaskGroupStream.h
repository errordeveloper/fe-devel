/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MT_TASK_GROUP_STREAM_H
#define _FABRIC_MT_TASK_GROUP_STREAM_H

#include "TaskGroup.h"
#include <vector>

namespace Fabric
{
  namespace MT
  {
    class TaskGroupStream
    {
    public:
    
      TaskGroupStream();
      
      void clear();
      void add( unsigned rank, TaskBase const *task );
      
      void execute( RC::Handle<LogCollector> const &logCollector, void *userdata ) const;
      
      std::string desc() const;
    
    private:
    
      std::vector< TaskGroup > m_taskGroups;
    };
  };
};

#endif //_FABRIC_MT_TASK_GROUP_STREAM_H
