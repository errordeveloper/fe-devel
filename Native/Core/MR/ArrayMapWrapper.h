/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_MR_ARRAY_MAP_WRAPPER_H
#define _FABRIC_MR_ARRAY_MAP_WRAPPER_H

#include <Fabric/Core/MR/ArrayProducerWrapper.h>

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
    class ArrayMapOperatorWrapper;
  }
  
  namespace MR
  {
    class ValueProducerWrapper;
    class ArrayProducerWrapper;
    class ArrayMap;
    
    class ArrayMapWrapper : public ArrayProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
    public:
    
      static RC::Handle<ArrayMapWrapper> Create(
        RC::ConstHandle<ArrayProducerWrapper> const &inputArrayProducer,
        RC::ConstHandle<KLC::ArrayMapOperatorWrapper> const &operator_,
        RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
        );
      
      virtual RC::ConstHandle<ArrayProducer> getUnwrapped() const;
      
      // Virtual functions: ArrayProducer
    
    protected:
        
      ArrayMapWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<ArrayProducerWrapper> const &inputArrayProducer,
        RC::ConstHandle<KLC::ArrayMapOperatorWrapper> const &operator_,
        RC::ConstHandle<ValueProducerWrapper> const &sharedValueProducer
        );
    
      virtual char const *getKind() const;
      virtual void toJSONImpl( Util::JSONObjectGenerator &jog ) const;
    
    private:
    
      RC::ConstHandle<ArrayProducerWrapper> m_inputArrayProducer;
      RC::ConstHandle<KLC::ArrayMapOperatorWrapper> m_operator;
      RC::ConstHandle<ValueProducerWrapper> m_sharedValueProducer;
    
      RC::ConstHandle<ArrayMap> m_unwrapped;
    };
  }
}

#endif //_FABRIC_MR_ARRAY_MAP_WRAPPER_H
