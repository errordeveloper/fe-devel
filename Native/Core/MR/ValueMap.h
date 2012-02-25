/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_MAP_H
#define _FABRIC_MR_VALUE_MAP_H

#include <Fabric/Core/MR/ValueProducer.h>

#include <stdint.h>
#include <vector>

namespace Fabric
{
  namespace MR
  {
    class ValueIOOperator;
    
    class ValueMap : public ValueProducer
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ValueMap> Create(
        RC::ConstHandle<ValueProducer> const &input,
        RC::ConstHandle<ValueIOOperator> const &operator_,
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
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<ValueMap> const &valueMap );
      
        virtual void produce( void *data ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<ValueMap> const &valueMap );
        ~ComputeState();
        
      private:
      
        RC::ConstHandle<ValueProducer::ComputeState> m_inputComputeState;
        RC::ConstHandle<RT::Desc> m_inputDesc;
        RC::ConstHandle<ValueIOOperator> m_operator;
        RC::ConstHandle<ValueProducer> m_shared;
        std::vector<uint8_t> m_sharedData;
      };
      
      ValueMap(
        RC::ConstHandle<ValueProducer> const &input,
        RC::ConstHandle<ValueIOOperator> const &operator_,
        RC::ConstHandle<ValueProducer> const &shared
        );
    
    private:
    
      RC::ConstHandle<ValueProducer> m_input;
      RC::ConstHandle<ValueIOOperator> m_operator;
      RC::ConstHandle<ValueProducer> m_shared;
    };
  };
};

#endif //_FABRIC_MR_VALUE_MAP_H
