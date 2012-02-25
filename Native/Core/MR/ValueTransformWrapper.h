/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_TRANSFORM_WRAPPER_H
#define _FABRIC_MR_VALUE_TRANSFORM_WRAPPER_H

#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Base/RC/ConstHandle.h>

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
    class ValueTransformOperatorWrapper;
  }
  
  namespace MR
  {
    class ValueProducerWrapper;
    class ValueTransform;
    
    class ValueTransformWrapper : public ValueProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()

    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ValueTransformWrapper> Create(
        RC::ConstHandle<ValueProducerWrapper> const &inputValueProducer,
        RC::ConstHandle<KLC::ValueTransformOperatorWrapper> const &operator_,
        RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
        );
      
      virtual RC::ConstHandle<ValueProducer> getUnwrapped() const;
      
      // Virtual functions: ValueProducer
    
    protected:
        
      ValueTransformWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<ValueProducerWrapper> const &inputValueProducer,
        RC::ConstHandle<KLC::ValueTransformOperatorWrapper> const &operator_,
        RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
        );
    
      virtual char const *getKind() const;
      virtual void toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const;
    
    private:
    
      RC::ConstHandle<ValueProducerWrapper> m_inputValueProducer;
      RC::ConstHandle<KLC::ValueTransformOperatorWrapper> m_operator;
      RC::ConstHandle<ValueProducerWrapper> m_sharedValueProducer;
    
      RC::ConstHandle<ValueTransform> m_unwrapped;
    };
  }
}

#endif //_FABRIC_MR_VALUE_TRANSFORM_WRAPPER_H
