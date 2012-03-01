/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_TRANSFORM_H
#define _FABRIC_MR_VALUE_TRANSFORM_H

#include <Fabric/Core/MR/ValueProducer.h>

#include <stdint.h>
#include <vector>

namespace Fabric
{
  namespace MR
  {
    class ValueOutputOperator;
    
    class ValueTransform : public ValueProducer
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ValueTransform> Create(
        RC::ConstHandle<ValueProducer> const &input,
        RC::ConstHandle<ValueOutputOperator> const &operator_,
        RC::ConstHandle<ValueProducer> const &shared
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
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<ValueTransform> const &valueTransform );
      
        virtual void produce( void *data ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<ValueTransform> const &valueTransform );
        ~ComputeState();
        
      private:
      
        RC::ConstHandle<ValueTransform> m_valueTransform;
        RC::ConstHandle<ValueProducer::ComputeState> m_inputValueProducerComputeState;
        std::vector<uint8_t> m_sharedData;
      };
    
      ValueTransform(
        RC::ConstHandle<ValueProducer> const &input,
        RC::ConstHandle<ValueOutputOperator> const &operator_,
        RC::ConstHandle<ValueProducer> const &shared
        );
    
    private:
    
      RC::ConstHandle<ValueProducer> m_input;
      RC::ConstHandle<ValueOutputOperator> m_operator;
      RC::ConstHandle<ValueProducer> m_shared;
      
      RC::ConstHandle<RT::Desc> m_valueDesc;
    };
  };
};

#endif //_FABRIC_MR_VALUE_TRANSFORM_H
