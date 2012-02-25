/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_PRODUCER_WRAPPER_H
#define _FABRIC_MR_VALUE_PRODUCER_WRAPPER_H

#include <Fabric/Core/MR/ProducerWrapper.h>

namespace Fabric
{
  namespace MR
  {
    class ValueProducer;
    
    class ValueProducerWrapper : public ProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()

      // Virtual functions: GC::Object
      
    public:
      REPORT_RC_LEAKS
      
      virtual RC::ConstHandle<ValueProducer> getUnwrapped() const = 0;

      virtual void jsonExec(
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
    
    protected:
    
      ValueProducerWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM
        );
    
      void jsonExecProduce(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );

      void jsonExecProduceAsync(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );

      void jsonExecFlush(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );

    private:
    
      static void JSONExecProduceAsyncFinishedCallback( void *userdata );
    };
  }
}

#endif //_FABRIC_MR_VALUE_PRODUCER_WRAPPER_H
