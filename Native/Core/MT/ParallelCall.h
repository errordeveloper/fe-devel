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
      
      void evalWithArgs( void * const *argv, void (*functionPtr)( ... ), void *userdata ) const
      {
        Util::TLSVar<void *>::Setter userdataSetter( s_userdataTLS, userdata );
        
        size_t argc = m_paramCount;
        switch ( argc )
        {
          case 0:
            functionPtr();
            break;
          case 1:
            functionPtr(
              argv[ 0]
              );
            break;
          case 2:
            functionPtr(
              argv[ 0], argv[ 1]
              );
            break;
          case 3:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2]
              );
            break;
          case 4:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3]
              );
            break;
          case 5:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4]
              );
            break;
          case 6:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5]
              );
            break;
          case 7:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6]
              );
            break;
          case 8:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7]
              );
            break;
          case 9:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8]
              );
            break;
          case 10:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9]
              );
            break;
          case 11:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10]
              );
            break;
          case 12:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11]
              );
            break;
          case 13:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12]
              );
            break;
          case 14:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13]
              );
            break;
          case 15:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14]
              );
            break;
          case 16:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15]
              );
            break;
          case 17:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16]
              );
            break;
          case 18:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17]
              );
            break;
          case 19:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18]
              );
            break;
          case 20:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19]
              );
            break;
          case 21:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20]
              );
            break;
          case 22:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21]
              );
            break;
          case 23:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22]
              );
            break;
          case 24:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22], argv[23]
              );
            break;
          case 25:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22], argv[23],
              argv[24]
              );
            break;
          case 26:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22], argv[23],
              argv[24], argv[25]
              );
            break;
          case 27:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22], argv[23],
              argv[24], argv[25], argv[26]
              );
            break;
          case 28:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22], argv[23],
              argv[24], argv[25], argv[26], argv[27]
              );
            break;
          case 29:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22], argv[23],
              argv[24], argv[25], argv[26], argv[27],
              argv[28]
              );
            break;
          case 30:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22], argv[23],
              argv[24], argv[25], argv[26], argv[27],
              argv[28], argv[29]
              );
            break;
          case 31:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22], argv[23],
              argv[24], argv[25], argv[26], argv[27],
              argv[28], argv[29], argv[30]
              );
            break;
          case 32:
            functionPtr(
              argv[ 0], argv[ 1], argv[ 2], argv[ 3],
              argv[ 4], argv[ 5], argv[ 6], argv[ 7],
              argv[ 8], argv[ 9], argv[10], argv[11],
              argv[12], argv[13], argv[14], argv[15],
              argv[16], argv[17], argv[18], argv[19],
              argv[20], argv[21], argv[22], argv[23],
              argv[24], argv[25], argv[26], argv[27],
              argv[28], argv[29], argv[30], argv[31]
              );
            break;
          default:
            throw Exception( "a maximum of %u parameters are supported for operator calls", FABRIC_MT_PARALLEL_CALL_MAX_PARAM_COUNT );
        }
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
