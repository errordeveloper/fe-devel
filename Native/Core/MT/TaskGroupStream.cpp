/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "TaskGroupStream.h"

namespace Fabric
{
  namespace MT
  {
    TaskGroupStream::TaskGroupStream()
    {
    }
    
    void TaskGroupStream::clear()
    {
      for ( std::vector< TaskGroup >::iterator it=m_taskGroups.begin(); it!=m_taskGroups.end(); ++it )
        it->clear();
    }
    
    void TaskGroupStream::add( unsigned rank, TaskBase const *task )
    {
      if ( rank >= m_taskGroups.size() )
        m_taskGroups.resize( rank+1 );
      m_taskGroups[rank].add( task );
    }
    
    void TaskGroupStream::execute( RC::Handle<LogCollector> const &logCollector, void *userdata ) const
    {
      for ( std::vector< TaskGroup >::const_iterator it=m_taskGroups.begin(); it!=m_taskGroups.end(); ++it )
        it->execute( logCollector, userdata );
    }

    std::string TaskGroupStream::desc() const
    {
      std::string result = "[";
      for ( std::vector< TaskGroup >::const_iterator it=m_taskGroups.begin(); it!=m_taskGroups.end(); ++it )
      {
        if ( result.length() > 1 )
          result += ",";
        result += it->desc();
      }
      return result + "]";
    }
  };
};
