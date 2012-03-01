/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MT_TASK_H
#define _FABRIC_MT_TASK_H

namespace Fabric
{
  namespace MT
  {
    class TaskBase
    {
    public:
    
      virtual void execute( void *userdata ) const = 0;
    };
    
    template< class T > class Task : public TaskBase
    {
    public:
    
      typedef void (T::*ExecuteCallback)( void *userdata );
      
      Task(  T *t, ExecuteCallback executeCallback )
        : m_t( t )
        , m_executeCallback( executeCallback )
      {
      }

      virtual void execute( void *userdata ) const
      {
        (m_t->*m_executeCallback)( userdata );
      }
    
    private:
    
      T *m_t;
      ExecuteCallback m_executeCallback;
    };
  };
};

#endif //_FABRIC_MT_TASK_H
