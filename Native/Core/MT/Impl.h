/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_MT_IMPL_H
#define _FABRIC_MT_IMPL_H

#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/Util/Cond.h>
#include <Fabric/Core/Util/Debug.h>
#include <Fabric/Core/Util/Mutex.h>
#include <Fabric/Core/Util/Thread.h>
#include <Fabric/Core/Util/TLS.h>

#include <vector>

namespace Fabric
{
  namespace MT
  {
    class LogCollector;

    class ThreadPool
    {
      class Task 
      {
      public:
      
        typedef void (*FinishedCallback)( void * );
      
        Task(
          bool needDelete,
          RC::Handle<LogCollector> const &logCollector,
          size_t count,
          void (*callback)( void *, size_t ),
          void *userdata,
          FinishedCallback finishedCallback = 0,
          void *finishedUserdata = 0
          )
          : m_logCollector( logCollector )
          , m_count( count )
          , m_callback( callback )
          , m_userdata( userdata )
          , m_finishedCallback( finishedCallback )
          , m_finishedUserdata( finishedUserdata )
          , m_nextIndex( 0 )
          , m_completedCount( 0 )
          , m_needDelete( needDelete )
        {
        }
        
        ~Task()
        {
          FABRIC_ASSERT( m_nextIndex == m_count );
          FABRIC_ASSERT( m_completedCount == m_count );
        }
        
        void dispose()
        {
          if ( m_needDelete )
            delete this;
        }
        
        void preExecute_CRITICAL_SECTION( size_t &index, bool &keep )
        {
          FABRIC_ASSERT( m_nextIndex < m_count );
          index = m_nextIndex++;
          keep = m_nextIndex < m_count;
        }
        
        void execute( size_t index )
        {
          m_callback( m_userdata, index );
        }
        
        void postExecute_CRITICAL_SECTION()
        {
          FABRIC_ASSERT( m_completedCount < m_count );
          ++m_completedCount;
        }
        
        bool completed_CRITICAL_SECTION() const
        {
          return m_completedCount == m_count;
        }

        RC::Handle<LogCollector> getLogCollector() const
        {
          return m_logCollector;
        }
        
        FinishedCallback getFinishedCallback() const
        {
          return m_finishedCallback;
        }
        
        void *getFinishedUserdata() const
        {
          return m_finishedUserdata;
        }
        
      private:
        
        RC::Handle<LogCollector> m_logCollector;
        size_t m_count;
        void (*m_callback)( void *, size_t );
        void *m_userdata;
        
        FinishedCallback m_finishedCallback;
        void *m_finishedUserdata;

        size_t m_nextIndex;
        size_t m_completedCount;
        
        bool m_needDelete;
      };
      
    public:

      static const size_t Idle = 1;
      static const size_t MainThreadOnly = 2;
      
      static ThreadPool *Instance();

      ThreadPool();
      ~ThreadPool();
      
      void executeParallel(
        RC::Handle<LogCollector> const &logCollector,
        size_t count,
        void (*callback)( void *userdata, size_t index ),
        void *userdata,
        size_t flags
        );
      
      void executeParallelAsync(
        RC::Handle<LogCollector> const &logCollector,
        size_t count,
        void (*callback)( void *userdata, size_t index ),
        void *userdata,
        size_t flags,
        void (*finishedCallback)( void * ),
        void *finishedUserdata
        );
      
      void terminate();
      
      bool isMainThread() const
      {
        return m_isMainThread.get();
      }
        
    protected:
    
      void executeOneTaskIfPossible_CRITICAL_SECTION();

      void workerMain();    
        
    private:
    
      static void WorkerMainCallback( void *_this );

      Util::Mutex m_stateMutex;
      Util::Cond m_stateCond;
      std::vector<Util::Thread> m_workerThreads;
      std::vector<Task *> m_tasks;
      Util::TLSVar<bool> m_isMainThread;
      std::vector<Task *> m_mainThreadTasks;
      std::vector<Task *> m_idleTasks;
      bool m_running;
    };
    
    inline void executeParallel(
      RC::Handle<LogCollector> const &logCollector,
      size_t count,
      void (*callback)( void *userdata, size_t index ),
      void *userdata,
      bool mainThreadOnly
      )
    {
      ThreadPool::Instance()->executeParallel( logCollector, count, callback, userdata, mainThreadOnly? ThreadPool::MainThreadOnly: 0 );
    }
  };
};

#endif //_FABRIC_MT_IMPL_H
