/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_ARRAY_MAP_H
#define _FABRIC_MR_ARRAY_MAP_H

#include <Fabric/Core/MR/ArrayProducer.h>

#include <stdint.h>
#include <vector>

namespace Fabric
{
  namespace MR
  {
    class ArrayIOOperator;
    class ValueProducer;
    
    class ArrayMap : public ArrayProducer
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ArrayMap> Create(
        RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
        RC::ConstHandle<ArrayIOOperator> const &arrayIOOperator,
        RC::ConstHandle<ValueProducer> const &sharedValueProducer
        );
      
      // Virtual functions: ArrayProducer
    
    public:
      
      virtual RC::ConstHandle<RT::Desc> getElementDesc() const;
      virtual const RC::Handle<ArrayProducer::ComputeState> createComputeState() const;
      virtual void flush();
            
    protected:
    
      class ComputeState : public ArrayProducer::ComputeState
      {
      public:
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<ArrayMap> const &arrayMap );
      
        virtual void produce( size_t index, void *data ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<ArrayMap> const &arrayMap );
        ~ComputeState();
        
      private:
      
        RC::ConstHandle<ArrayMap> m_arrayMap;
        RC::ConstHandle<ArrayProducer::ComputeState> m_inputArrayProducerComputeState;
        std::vector<uint8_t> m_sharedData;
      };
    
      ArrayMap(
        RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
        RC::ConstHandle<ArrayIOOperator> const &arrayIOOperator,
        RC::ConstHandle<ValueProducer> const &sharedValueProducer
        );
    
    private:
    
      RC::ConstHandle<ArrayProducer> m_inputArrayProducer;
      RC::ConstHandle<ArrayIOOperator> m_arrayIOOperator;
      RC::ConstHandle<ValueProducer> m_sharedValueProducer;
    };
  };
};

#endif //_FABRIC_MR_ARRAY_MAP_H
