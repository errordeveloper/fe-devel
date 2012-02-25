/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_REDUCE_WRAPPER_H
#define _FABRIC_MR_REDUCE_WRAPPER_H

#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace KLC
  {
    class ReduceOperatorWrapper;
  }
  
  namespace MR
  {
    class ArrayProducerWrapper;
    
    class ReduceWrapper : public ValueProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
    
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ReduceWrapper> Create(
        RC::ConstHandle<ArrayProducerWrapper> const &inputWrapper,
        RC::ConstHandle<KLC::ReduceOperatorWrapper> const &operatorWrapper,
        RC::ConstHandle<ValueProducerWrapper> const &sharedWrapper
        );
      
      virtual RC::ConstHandle<ValueProducer> getUnwrapped() const;
            
    protected:
      
      ReduceWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<ArrayProducerWrapper> const &inputWrapper,
        RC::ConstHandle<KLC::ReduceOperatorWrapper> const &operatorWrapper,
        RC::ConstHandle<ValueProducerWrapper> const &sharedWrapper
        );
    
      virtual char const *getKind() const;
      virtual void toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const;
    
    private:
    
      RC::ConstHandle<ArrayProducerWrapper> m_inputWrapper;
      RC::ConstHandle<KLC::ReduceOperatorWrapper> m_operatorWrapper;
      RC::ConstHandle<ValueProducerWrapper> m_sharedWrapper;
      
      RC::ConstHandle<ValueProducer> m_unwrapped;
    };
  }
}

#endif //_FABRIC_MR_REDUCE_WRAPPER_H
