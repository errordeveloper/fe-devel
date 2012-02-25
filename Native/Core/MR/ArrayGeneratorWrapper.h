/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_ARRAY_GENERATOR_WRAPPER_H
#define _FABRIC_MR_ARRAY_GENERATOR_WRAPPER_H

#include <Fabric/Core/MR/ArrayProducerWrapper.h>

namespace Fabric
{
  namespace JSON
  {
    class ObjectEncoder;
  }
  
  namespace RT
  {
    class Desc;
    class Manager;
  }
  
  namespace JSON
  {
    class Array;
  }
  
  namespace KLC
  {
    class ArrayGeneratorOperatorWrapper;
  }
  
  namespace MR
  {
    class ValueProducerWrapper;
    class ArrayProducerWrapper;
    class ArrayGenerator;
    
    class ArrayGeneratorWrapper : public ArrayProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ArrayGeneratorWrapper> Create(
        RC::ConstHandle<ValueProducerWrapper> const &countValueProducer,
        RC::ConstHandle<KLC::ArrayGeneratorOperatorWrapper> const &operator_,
        RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
        );
      
      virtual RC::ConstHandle<ArrayProducer> getUnwrapped() const;
      
      // Virtual functions: ArrayProducer
    
    protected:
        
      ArrayGeneratorWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<ValueProducerWrapper> const &countValueProducer,
        RC::ConstHandle<KLC::ArrayGeneratorOperatorWrapper> const &operator_,
        RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
        );
    
      virtual char const *getKind() const;
      virtual void toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const;
    
    private:
    
      RC::ConstHandle<ValueProducerWrapper> m_countValueProducer;
      RC::ConstHandle<KLC::ArrayGeneratorOperatorWrapper> m_operator;
      RC::ConstHandle<ValueProducerWrapper> m_sharedValueProducer;
    
      RC::ConstHandle<ArrayGenerator> m_unwrapped;
    };
  }
}

#endif //_FABRIC_MR_ARRAY_GENERATOR_WRAPPER_H
