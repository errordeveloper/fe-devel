/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_PRODUCER_H
#define _FABRIC_MR_VALUE_PRODUCER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/MR/Producer.h>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
    class ObjectDecoder;
    class ObjectEncoder;
  };
  
  namespace RT
  {
    class Desc;
  };
  
  namespace MR
  {
    class ValueProducer : public Producer
    {
    public:
      REPORT_RC_LEAKS
    
      class ComputeState : public RC::Object
      {
      public:
     
        virtual void produce( void *data ) const = 0;
        virtual void produceJSON( JSON::Encoder &jg ) const;
        void produceJSONAsync(
          JSON::ObjectEncoder &jsonObjectEncoder,
          void (*finishedCallback)( void * ),
          void *finishedUserdata
          );
        
      protected:
      
        ComputeState( RC::ConstHandle<ValueProducer> const &valueProducer );
    
        static void ProduceJSONAsyncCallback(
          void *userdata,
          size_t index
          );
        
      protected:
      
        RC::ConstHandle<ValueProducer> m_valueProducer;
      };

      virtual RC::ConstHandle<RT::Desc> getValueDesc() const = 0;
      virtual const RC::Handle<ComputeState> createComputeState() const = 0;
    
    protected:
    
      ValueProducer();
    };
  }
}

#endif //_FABRIC_MR_VALUE_PRODUCER_H
