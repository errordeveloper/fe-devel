/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_MR_CONST_VALUE_WRAPPER_H
#define _FABRIC_MR_CONST_VALUE_WRAPPER_H

#include <Fabric/Core/MR/ValueProducerWrapper.h>

namespace Fabric
{
  namespace RT
  {
    class Manager;
  };
  
  namespace MR
  {
    class ConstValue;
    
    class ConstValueWrapper : public ValueProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()

    public:
    
      static RC::Handle<ConstValueWrapper> Create(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &elementDesc,
        RC::ConstHandle<JSON::Value> const &jsonValue
        );
      
      virtual RC::ConstHandle<ValueProducer> getUnwrapped() const;
    
    public:
      
      ConstValueWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &elementDesc,
        RC::ConstHandle<JSON::Value> const &jsonValue
        );
    
      virtual char const *getKind() const;
      virtual void toJSONImpl( Util::JSONObjectGenerator &jog ) const;
    
    private:
    
      RC::ConstHandle<ConstValue> m_unwrapped;
    };
  };
};

#endif //_FABRIC_MR_CONST_VALUE_WRAPPER_H