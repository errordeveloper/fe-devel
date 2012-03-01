/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_ARRAY_GENERATOR_H
#define _FABRIC_MR_ARRAY_GENERATOR_H

#include <Fabric/Core/MR/ArrayProducer.h>
#include <Fabric/Core/MR/ValueProducer.h>

#include <stdint.h>
#include <vector>

namespace Fabric
{
  namespace MR
  {
    class ArrayOutputOperator;
    
    class ArrayGenerator : public ArrayProducer
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ArrayGenerator> Create(
        RC::ConstHandle<ValueProducer> const &countValueProducer,
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
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<ArrayGenerator> const &arrayGenerator );
      
        virtual void produce( size_t index, void *data ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<ArrayGenerator> const &arrayGenerator );
        ~ComputeState();
        
      private:
      
        RC::ConstHandle<ArrayGenerator> m_arrayGenerator;
        std::vector<uint8_t> m_sharedData;
      };
    
      ArrayGenerator(
        RC::ConstHandle<ValueProducer> const &countValueProducer,
        RC::ConstHandle<ArrayOutputOperator> const &operator_,
        RC::ConstHandle<ValueProducer> const &sharedValueProducer
        );
    
    private:
    
      RC::ConstHandle<ValueProducer> m_countValueProducer;
      RC::ConstHandle<ArrayOutputOperator> m_operator;
      RC::ConstHandle<ValueProducer> m_sharedValueProducer;
    };
  };
};

#endif //_FABRIC_MR_ARRAY_GENERATOR_H
