/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_REDUCE_H
#define _FABRIC_MR_REDUCE_H

#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Core/MR/ArrayProducer.h>
#include <Fabric/Core/Util/Mutex.h>

namespace Fabric
{
  namespace KLC
  {
  };
  
  namespace MR
  {
    class ArrayIOOperator;

    class Reduce : public ValueProducer
    {
      class Execution;
      
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<Reduce> Create(
        RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
        RC::ConstHandle<ArrayIOOperator> const &operator_,
        RC::ConstHandle<ValueProducer> const &sharedValueProducer
        );
      
      // Virtual functions: ValueProducer
    
    public:
      
      virtual RC::ConstHandle<RT::Desc> getValueDesc() const;
      virtual const RC::Handle<ValueProducer::ComputeState> createComputeState() const;
      virtual void flush();
            
    protected:
    
      class ComputeState : public ValueProducer::ComputeState
      {
      public:
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<Reduce> const &reduce );
      
        virtual void produce( void *data ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<Reduce> const &reduce );
        ~ComputeState();
        
      private:
      
        RC::ConstHandle<ArrayProducer::ComputeState> m_inputComputeState;
        RC::ConstHandle<RT::Desc> m_inputDesc;
        RC::ConstHandle<ArrayIOOperator> m_operator;
        RC::ConstHandle<ValueProducer> m_shared;
        std::vector<uint8_t> m_sharedData;
        Util::Mutex &m_mutex;
      };
      
      Reduce(
        RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
        RC::ConstHandle<ArrayIOOperator> const &reduceOperator,
        RC::ConstHandle<ValueProducer> const &sharedValueProducer
        );
    
    private:
    
      RC::ConstHandle<ArrayProducer> m_inputArrayProducer;
      RC::ConstHandle<ArrayIOOperator> m_operator;
      RC::ConstHandle<ValueProducer> m_sharedValueProducer;
      
      mutable Util::Mutex m_mutex;
    };
  };
};

#endif //_FABRIC_MR_REDUCE_H
