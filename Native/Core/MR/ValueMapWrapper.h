/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_MAP_WRAPPER_H
#define _FABRIC_MR_VALUE_MAP_WRAPPER_H

#include <Fabric/Core/MR/ValueProducerWrapper.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace KLC
  {
    class ValueMapOperatorWrapper;
  }
  
  namespace MR
  {
    class ValueMapWrapper : public ValueProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
    
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ValueMapWrapper> Create(
        RC::ConstHandle<ValueProducerWrapper> const &inputWrapper,
        RC::ConstHandle<KLC::ValueMapOperatorWrapper> const &operatorWrapper,
        RC::ConstHandle<ValueProducerWrapper> const &sharedWrapper
        );
      
      virtual RC::ConstHandle<ValueProducer> getUnwrapped() const;
            
    protected:
      
      ValueMapWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<ValueProducerWrapper> const &inputWrapper,
        RC::ConstHandle<KLC::ValueMapOperatorWrapper> const &operatorWrapper,
        RC::ConstHandle<ValueProducerWrapper> const &sharedWrapper
        );
    
      virtual char const *getKind() const;
      virtual void toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const;
    
    private:
    
      RC::ConstHandle<ValueProducerWrapper> m_inputWrapper;
      RC::ConstHandle<KLC::ValueMapOperatorWrapper> m_operatorWrapper;
      RC::ConstHandle<ValueProducerWrapper> m_sharedWrapper;
      
      RC::ConstHandle<ValueProducer> m_unwrapped;
    };
  }
}

#endif //_FABRIC_MR_VALUE_MAP_WRAPPER_H
