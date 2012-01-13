/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_MR_VALUE_GENERATOR_WRAPPER_H
#define _FABRIC_MR_VALUE_GENERATOR_WRAPPER_H

#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace Util
  {
    class JSONObjectGenerator;
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
    class ValueGeneratorOperatorWrapper;
  }
  
  namespace MR
  {
    class ValueProducerWrapper;
    class ValueGenerator;
    
    class ValueGeneratorWrapper : public ValueProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()

    public:
    
      static RC::Handle<ValueGeneratorWrapper> Create(
        RC::ConstHandle<KLC::ValueGeneratorOperatorWrapper> const &operator_,
        RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
        );
      
      virtual RC::ConstHandle<ValueProducer> getUnwrapped() const;
      
      // Virtual functions: ValueProducer
    
    protected:
        
      ValueGeneratorWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<KLC::ValueGeneratorOperatorWrapper> const &operator_,
        RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
        );
    
      virtual char const *getKind() const;
      virtual void toJSONImpl( Util::JSONObjectGenerator &jog ) const;
    
    private:
    
      RC::ConstHandle<KLC::ValueGeneratorOperatorWrapper> m_operator;
      RC::ConstHandle<ValueProducerWrapper> m_sharedValueProducer;
    
      RC::ConstHandle<ValueGenerator> m_unwrapped;
    };
  }
}

#endif //_FABRIC_MR_VALUE_GENERATOR_WRAPPER_H
