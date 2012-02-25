/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_ARRAY_TRANSFORM_H
#define _FABRIC_MR_ARRAY_TRANSFORM_H

#include <Fabric/Core/MR/ArrayProducer.h>
#include <Fabric/Core/MR/ValueProducer.h>

#include <stdint.h>
#include <vector>

namespace Fabric
{
  namespace MR
  {
    class ArrayOutputOperator;
    
    class ArrayTransform : public ArrayProducer
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ArrayTransform> Create(
        RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
        RC::ConstHandle<ArrayOutputOperator> const &operator_,
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
        REPORT_RC_LEAKS
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<ArrayTransform> const &arrayTransform );
      
        virtual void produce( size_t index, void *data ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<ArrayTransform> const &arrayTransform );
        ~ComputeState();
        
      private:
      
        RC::ConstHandle<ArrayTransform> m_arrayTransform;
        RC::ConstHandle<ArrayProducer::ComputeState> m_inputArrayProducerComputeState;
        std::vector<uint8_t> m_sharedData;
      };
    
      ArrayTransform(
        RC::ConstHandle<ArrayProducer> const &inputArrayProducer,
        RC::ConstHandle<ArrayOutputOperator> const &operator_,
        RC::ConstHandle<ValueProducer> const &sharedValueProducer
        );
    
    private:
    
      RC::ConstHandle<ArrayProducer> m_inputArrayProducer;
      RC::ConstHandle<ArrayOutputOperator> m_operator;
      RC::ConstHandle<ValueProducer> m_sharedValueProducer;
    };
  };
};

#endif //_FABRIC_MR_ARRAY_TRANSFORM_H
