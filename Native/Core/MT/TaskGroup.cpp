/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "TaskGroup.h"
#include "Task.h"
#include "Impl.h"

namespace Fabric
{
  namespace MT
  {
    TaskGroup::TaskGroup()
    {
    }

    TaskGroup::TaskGroup( TaskGroup const &that )
      : m_tasks( that.m_tasks )
    {
    }
    
    TaskGroup::~TaskGroup()
    {
    }
    
    TaskGroup &TaskGroup::operator =( TaskGroup const &that )
    {
      m_tasks = that.m_tasks;
      return *this;
    }
      
    void TaskGroup::clear()
    {
      m_tasks.resize(0);
    }
        
    void TaskGroup::add( TaskBase const *task )
    {
      m_tasks.push_back( task );
    }
    
    void TaskGroup::executeTask( void *userdata, unsigned index ) const
    {
      m_tasks[index]->execute( userdata );
    }
    
    void TaskGroup::ExecuteTask( void *_context, size_t index )
    {
      Context *context = static_cast<Context *>( _context );
      context->taskGroup->executeTask( context->userdata, index );
    }

    void TaskGroup::execute( RC::Handle<LogCollector> const &logCollector, void *userdata ) const
    {
      if ( !m_tasks.empty() )
      {
        Context context;
        context.taskGroup = this;
        context.userdata = userdata;
        executeParallel( logCollector, m_tasks.size(), &TaskGroup::ExecuteTask, &context, false );
      }
    }

    std::string TaskGroup::desc() const
    {
      char buf[4096];
      snprintf( buf, 4096, "%u tasks", (unsigned)m_tasks.size() );
      return buf;
    }
  };
};
