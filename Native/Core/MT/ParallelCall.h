/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MT_PARALLEL_CALL_H
#define _FABRIC_MT_PARALLEL_CALL_H

#include <Fabric/Core/MT/Impl.h>
#include <Fabric/Core/MT/Function.h>
#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/MT/Util.h>
#include <Fabric/Core/Util/Debug.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Core/Util/TLS.h>

#include <vector>
#include <stdint.h>

#if defined( FABRIC_WIN32 )
#include <malloc.h>
#else
#include <alloca.h>
#endif

#define FABRIC_MT_PARALLEL_CALL_MAX_PARAM_COUNT 32

namespace Fabric
{
  namespace MT
  {
    class ParallelCall : public RC::Object
    {
      struct ParallelExecutionUserData
      {
        ParallelCall const *parallelCall;
        void (*functionPtr)( ... );
        void *userdata;
      };
      
    public:
      REPORT_RC_LEAKS
    
      typedef void (*FunctionPtr)( ... );

      static RC::Handle<ParallelCall> Create(
        RC::ConstHandle<Function> const &function,
        size_t paramCount,
        std::string const &debugDesc
        )
      {
        return new ParallelCall( function, paramCount, debugDesc );
      }
    
    protected:

      ParallelCall( RC::ConstHandle<Function> const &function, size_t paramCount, std::string const &debugDesc );
      ~ParallelCall();
      
    public:
      
      void setBaseAddress( size_t index, void *baseAddress )
      {
        FABRIC_ASSERT( index < m_paramCount );
        m_baseAddresses[index] = baseAddress;
      }
      
      size_t addAdjustment( size_t count )
      {
        size_t index = m_adjustments.size();

        // andrew 2012-03-25
        // only allow a single adjustment unless there is a need to add
        // support for more later (e.g. for cross-product of adjustments)
        FABRIC_ASSERT( index == 0 );

        m_adjustments.resize( index + 1 );
        Adjustment &newAdjustment = m_adjustments[index];

        newAdjustment.count = count;
        newAdjustment.batchSize = std::max<size_t>( 1, count / (MT::getNumCores()*4) );
        newAdjustment.batchCount = (count + newAdjustment.batchSize - 1) / newAdjustment.batchSize;

        for ( size_t i=0; i<m_paramCount; ++i )
          newAdjustment.offsets[i] = 0;

        m_totalParallelCalls *= newAdjustment.batchCount;
        
        return index;
      }

      void setAdjustmentOffset( size_t adjustmentIndex, size_t paramIndex, size_t adjustmentOffset )
      {
        FABRIC_ASSERT( adjustmentIndex < m_adjustments.size() );
        FABRIC_ASSERT( paramIndex < m_paramCount );
        m_adjustments[adjustmentIndex].offsets[paramIndex] = adjustmentOffset;
      }
      
      void executeSerial( void *userdata ) const
      {
        //FABRIC_DEBUG_LOG( "executeSerial('%s')", m_debugDesc.c_str() );
        RC::ConstHandle<RC::Object> objectToAvoidFreeDuringExecution;
        void (*functionPtr)( ... ) = m_function->getFunctionPtr( objectToAvoidFreeDuringExecution );
        executeStub( NULL, functionPtr, userdata );
      }
      
      void executeParallel( RC::Handle<LogCollector> const &logCollector, void *userdata, bool mainThreadOnly ) const
      {
        //FABRIC_DEBUG_LOG( "executeParallel('%s')", m_debugDesc.c_str() );
        RC::ConstHandle<RC::Object> objectToAvoidFreeDuringExecution;
        ParallelExecutionUserData parallelExecutionUserData;
        parallelExecutionUserData.parallelCall = this;
        parallelExecutionUserData.functionPtr = m_function->getFunctionPtr( objectToAvoidFreeDuringExecution );
        parallelExecutionUserData.userdata = userdata;
        MT::executeParallel( logCollector, m_totalParallelCalls, &ParallelCall::ExecuteParallel, &parallelExecutionUserData, mainThreadOnly );
      }
      
      static void *GetUserdata()
      {
        return s_userdataTLS;
      }
      
    protected:

      void executeStub( size_t *iteration, void (*functionPtr)( ... ), void *userdata ) const
      {
        size_t start = 0;
        size_t count = 1;
        const void *offsets;
        if ( m_adjustments.size() == 0 )
          offsets = NULL;
        else
        {
          FABRIC_ASSERT( m_adjustments.size() == 1 );

          Adjustment const &adjustment = m_adjustments[0];
          if ( iteration )
          {
            size_t batch = *iteration % adjustment.batchCount;
            start = batch * adjustment.batchSize;
            count = adjustment.batchSize;
            if ( start + count > adjustment.count )
              count = adjustment.count - start;
          }
          else
            count = adjustment.count;

          offsets = adjustment.offsets;
        }

        Util::TLSVar<void *>::Setter userdataSetter( s_userdataTLS, userdata );
        functionPtr( start, count, m_baseAddresses, offsets );
      }
   
      static void ExecuteParallel( void *userdata, size_t iteration )
      {
        ParallelExecutionUserData const *parallelExecutionUserData = static_cast<ParallelExecutionUserData const *>( userdata );
        parallelExecutionUserData->parallelCall->executeStub( &iteration, parallelExecutionUserData->functionPtr, parallelExecutionUserData->userdata );
      }
      
    private:
    
      struct Adjustment
      {
        size_t count;
        size_t batchSize;
        size_t batchCount;
        size_t offsets[FABRIC_MT_PARALLEL_CALL_MAX_PARAM_COUNT];
      };
      
      RC::ConstHandle<Function> m_function;
      size_t m_paramCount;
      void *m_baseAddresses[FABRIC_MT_PARALLEL_CALL_MAX_PARAM_COUNT];
      std::vector<Adjustment> m_adjustments;
      size_t m_totalParallelCalls;
      std::string m_debugDesc;
      static Util::TLSVar<void *> s_userdataTLS;
    };
  };
};

#endif //_FABRIC_MT_PARALLEL_CALL_H
