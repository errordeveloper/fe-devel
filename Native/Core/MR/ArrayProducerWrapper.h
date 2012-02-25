/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_ARRAY_PRODUCER_WRAPPER_H
#define _FABRIC_MR_ARRAY_PRODUCER_WRAPPER_H

#include <Fabric/Core/MR/ProducerWrapper.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace RT
  {
    class Desc;
  };
  
  namespace MR
  {
    class ArrayProducer;
    
    class ArrayProducerWrapper : public ProducerWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
    public:
      REPORT_RC_LEAKS
      
      virtual RC::ConstHandle<ArrayProducer> getUnwrapped() const = 0;

      virtual void jsonExec(
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
    protected:
    
      ArrayProducerWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM
        );
    
      void jsonExecGetCount(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
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
  };
};

#endif //_FABRIC_MR_ARRAY_PRODUCER_WRAPPER_H
