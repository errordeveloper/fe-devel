/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_ARRAY_PRODUCER_H
#define _FABRIC_MR_ARRAY_PRODUCER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/MR/Producer.h>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
    class ObjectDecoder;
    class ObjectEncoder;
  }
  
  namespace RT
  {
    class Desc;
  }
  
  namespace MR
  {
    class ArrayProducer : public Producer
    {
      // Virtual functions: Object
      
      class Execution;
    
    public:
      REPORT_RC_LEAKS
    
      class ComputeState : public RC::Object
      {
      public:
        REPORT_RC_LEAKS
      
        size_t getCount() const
        {
          return m_count;
        }
        void produce( void *data ) const;
        void produceJSON( JSON::Encoder &jg ) const;
        virtual void produce( size_t index, void *data ) const = 0;
        virtual void produceJSON( size_t index, JSON::Encoder &jg ) const;
        void produce( size_t index, size_t count, void *data ) const;
        void produceJSON( size_t index, size_t count, JSON::Encoder &jg ) const;

        void produceJSONAsync(
          JSON::ObjectEncoder &jsonObjectEncoder,
          void (*finishedCallback)( void * ),
          void *finishedUserdata
          );
        void produceJSONAsync(
          size_t index,
          JSON::ObjectEncoder &jsonObjectEncoder,
          void (*finishedCallback)( void * ),
          void *finishedUserdata
          );
        void produceJSONAsync(
          size_t index,
          size_t count,
          JSON::ObjectEncoder &jsonObjectEncoder,
          void (*finishedCallback)( void * ),
          void *finishedUserdata
          );
      
      protected:
      
        ComputeState( RC::ConstHandle<ArrayProducer> const &arrayProducer );

        virtual void setCount( size_t count )
        {
          m_count = count;
        }
    
        static void ProduceJSONAsyncCallback_0(
          void *userdata,
          size_t index
          );
        static void ProduceJSONAsyncCallback_1(
          void *userdata,
          size_t index
          );
        static void ProduceJSONAsyncCallback_2(
          void *userdata,
          size_t index
          );
        
      private:
      
        RC::ConstHandle<ArrayProducer> m_arrayProducer;
        size_t m_count;
      };
            
      // Virtual functions: ArrayProducer
    
    public:
      
      virtual RC::ConstHandle<RT::Desc> getElementDesc() const = 0;
      virtual const RC::Handle<ComputeState> createComputeState() const = 0;
    
    protected:
    
      ArrayProducer();
    };
  }
}

#endif //_FABRIC_MR_ARRAY_PRODUCER_H
