/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/Reduce.h>
#include <Fabric/Core/MR/ArrayProducer.h>
#include <Fabric/Core/MR/ArrayIOOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/MT/Impl.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/MT/Util.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
    RC::Handle<Reduce> Reduce::Create(
      RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
      RC::ConstHandle<ArrayIOOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &sharedValueProducer
      )
    {
      return new Reduce( inputArrayProducer, operator_, sharedValueProducer );
    }
    
    Reduce::Reduce(
      RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
      RC::ConstHandle<ArrayIOOperator> const &operator_,
      RC::ConstHandle<ValueProducer> const &sharedValueProducer
      )
      : m_inputArrayProducer( inputArrayProducer )
      , m_operator( operator_ )
      , m_sharedValueProducer( sharedValueProducer )
      , m_mutex( "Reduce" )
    {
      RC::ConstHandle<RT::Desc> inputArrayProducerElementDesc = inputArrayProducer->getElementDesc();
      if ( !inputArrayProducerElementDesc )
        throw Exception("input array producer is invalid");
      RC::ConstHandle<RT::Desc> reduceOperatorInputDesc = operator_->getInputDesc();
      if ( !reduceOperatorInputDesc )
        throw Exception("reduce operator is invalid");
      if ( !reduceOperatorInputDesc->isEquivalentTo( inputArrayProducerElementDesc ) )
        throw Exception(
          "input element type ("
          + _(inputArrayProducerElementDesc->getUserName())
          + ") is not equivalent to reduce operator input type ("
          + _(reduceOperatorInputDesc->getUserName()) + ")"
          );
      RC::ConstHandle<RT::Desc> reduceOperatorSharedDesc = operator_->getSharedDesc();
      if ( reduceOperatorSharedDesc )
      {
        RC::ConstHandle<RT::Desc> sharedValueProducerValueDesc = sharedValueProducer->getValueDesc();
        if ( !sharedValueProducerValueDesc )
          throw Exception( "reduce operator requires a shared value but no shared value producer is provided" );
        if ( !sharedValueProducerValueDesc->isEquivalentTo( reduceOperatorSharedDesc ) )
          throw Exception(
            "shared value type ("
            + _(sharedValueProducerValueDesc->getUserName())
            + ") is not equivalent to reduce operator shared type ("
            + _(reduceOperatorSharedDesc->getUserName()) + ")"
            );
      }
    }
    
    RC::ConstHandle<RT::Desc> Reduce::getValueDesc() const
    {
      return m_operator->getOutputDesc();
    }
          
    const RC::Handle<ValueProducer::ComputeState> Reduce::createComputeState() const
    {
      return ComputeState::Create( this );
    }

    void Reduce::flush()
    {
      (const_cast<ArrayProducer *>(m_inputArrayProducer.ptr()))->flush();
    }
    
    RC::Handle<Reduce::ComputeState> Reduce::ComputeState::Create( RC::ConstHandle<Reduce> const &reduce )
    {
      return new ComputeState( reduce );
    }
    
    Reduce::ComputeState::ComputeState( RC::ConstHandle<Reduce> const &reduce )
      : ValueProducer::ComputeState( reduce )
      , m_inputComputeState( reduce->m_inputArrayProducer->createComputeState() )
      , m_inputDesc( reduce->getValueDesc() )
      , m_operator( reduce->m_operator )
      , m_shared( reduce->m_sharedValueProducer )
      , m_mutex( reduce->m_mutex )
    {
      if ( m_operator->takesSharedValue() )
      {
        m_sharedData.resize( m_shared->getValueDesc()->getAllocSize(), 0 );
        m_shared->createComputeState()->produce( &m_sharedData[0] );
      }
    }
    
    Reduce::ComputeState::~ComputeState()
    {
      if ( m_operator->takesSharedValue() )
        m_shared->getValueDesc()->disposeData( &m_sharedData[0] );
    }
    
    class Reduce::Execution
    {
    public:
    
      Execution(
        RC::ConstHandle<ArrayProducer::ComputeState> const &inputComputeState,
        RC::ConstHandle<RT::Desc> const &inputDesc,
        RC::ConstHandle<ArrayIOOperator> const &operator_,
        void *outputData,
        void const *sharedData,
        Util::Mutex &mutex
        )
        : m_inputComputeState( inputComputeState )
        , m_inputDesc( inputDesc )
        , m_operator( operator_ )
        , m_outputData( outputData )
        , m_sharedData( sharedData )
        , m_mutex( mutex )
        , m_logCollector( MT::tlsLogCollector.get() )
        , m_count( inputComputeState->getCount() )
        , m_numJobs( std::min( m_count, 4 * MT::getNumCores() ) )
        , m_indicesPerJob( (m_count + m_numJobs - 1) / m_numJobs )
      {
      }
      
      void run()
      {
        MT::executeParallel(
          m_logCollector,
          m_numJobs,
          &Callback,
          this,
          false
          );
      }
      
    protected:
      
      void callback( size_t jobIndex )
      {
        MT::TLSLogCollectorAutoSet logCollector( m_logCollector );
        
        bool takesIndex = m_operator->takesIndex();
        bool takesCount = m_operator->takesCount();
        bool takesSharedValue = m_operator->takesSharedValue();
        
        static const size_t maxGroupSize = 256;
        
        size_t inputElementSize = m_inputDesc->getAllocSize();
        size_t allInputElementsSize = maxGroupSize * inputElementSize;
        uint8_t *inputDatas = (uint8_t *)alloca( allInputElementsSize );

        size_t index = jobIndex * m_indicesPerJob;
        size_t endIndex = std::min( index + m_indicesPerJob, m_count );
        while ( index < endIndex )
        {
          memset( inputDatas, 0, allInputElementsSize );
          
          size_t groupSize = 0;
          while ( groupSize < maxGroupSize && index + groupSize < endIndex )
          {
            void *inputData = &inputDatas[groupSize * inputElementSize];
            m_inputComputeState->produce( index + groupSize, inputData );
            ++groupSize;
          }
          
          {
            Util::Mutex::Lock mutexLock( m_mutex );
            for ( size_t i=0; i<groupSize; ++i )
            {
              void *inputData = &inputDatas[i * inputElementSize];
              if ( takesSharedValue )
                m_operator->call( inputData, m_outputData, index + i, m_count, m_sharedData );
              else if ( takesCount )
                m_operator->call( inputData, m_outputData, index + i,m_count );
              else if ( takesIndex )
                m_operator->call( inputData, m_outputData, index + i );
              else
                m_operator->call( inputData, m_outputData );
            }
          }
        
          m_inputDesc->disposeDatas( inputDatas, groupSize, inputElementSize );
          
          index += groupSize;
        }
      }
      
      static void Callback( void *userdata, size_t jobIndex )
      {
        static_cast<Execution *>( userdata )->callback( jobIndex );
      }
      
    private:
    
      RC::ConstHandle<ArrayProducer::ComputeState> m_inputComputeState;
      RC::ConstHandle<RT::Desc> m_inputDesc;
      RC::ConstHandle<ArrayIOOperator> m_operator;
      void *m_outputData;
      void const *m_sharedData;
      Util::Mutex &m_mutex;
      RC::Handle<MT::LogCollector> m_logCollector;
      
      size_t m_count;
      size_t m_numJobs;
      size_t m_indicesPerJob;
    };
            
    void Reduce::ComputeState::produce( void *data ) const
    {
      Execution(
        m_inputComputeState,
        m_inputDesc,
        m_operator,
        data,
        &m_sharedData[0],
        m_mutex
        ).run();
    }
  }
}
