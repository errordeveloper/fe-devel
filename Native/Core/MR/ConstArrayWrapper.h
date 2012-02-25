/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_CONST_ARRAY_WRAPPER_H
#define _FABRIC_MR_CONST_ARRAY_WRAPPER_H

#include <Fabric/Core/MR/ArrayProducerWrapper.h>

namespace Fabric
{
  namespace JSON
  {
    struct Entity;
  }
  
  namespace RT
  {
    class Desc;
    class Manager;
  }
  
  namespace MR
  {
    class ArrayProducer;
    class ConstArray;
    
    class ConstArrayWrapper : public ArrayProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ConstArrayWrapper> Create(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &elementDesc,
        JSON::Entity const &entity
        );
      
      virtual RC::ConstHandle<ArrayProducer> getUnwrapped() const;
      
      // Virtual functions: ArrayProducer
    
    protected:
    
      ConstArrayWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &elementDesc,
        JSON::Entity const &entity
        );
    
      virtual char const *getKind() const;
      virtual void toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const;
    
    private:
    
      RC::ConstHandle<ConstArray> m_unwrapped;
    };
  }
}

#endif //_FABRIC_MR_CONST_ARRAY_WRAPPER_H
