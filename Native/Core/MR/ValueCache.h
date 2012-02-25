/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_CACHE_H
#define _FABRIC_MR_VALUE_CACHE_H

#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Core/Util/Mutex.h>

#include <stdint.h>
#include <vector>

namespace Fabric
{
  namespace RT
  {
    class Manager;
  }
  
  namespace MR
  {
    class ValueCache : public ValueProducer
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ValueCache> Create(
        RC::ConstHandle<ValueProducer> const &input
        );
      
    public:
      
      virtual RC::ConstHandle<RT::Desc> getValueDesc() const;
      virtual const RC::Handle<ValueProducer::ComputeState> createComputeState() const;
      
      virtual void flush();
            
    protected:
    
      class ComputeState : public ValueProducer::ComputeState
      {
      public:
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<ValueCache> const &valueMap );
      
        virtual void produce( void *data ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<ValueCache> const &valueMap );
        
      private:
      
        RC::ConstHandle<ValueCache> m_valueCache;
        RC::ConstHandle<ValueProducer::ComputeState> m_inputValueProducerComputeState;
        mutable Util::Mutex m_mutex;
      };
      
      ValueCache(
        RC::ConstHandle<ValueProducer> const &input
        );
      ~ValueCache();
    
    private:

      RC::ConstHandle<ValueProducer> m_input;
      
      RC::ConstHandle<RT::Desc> m_valueDesc;
      mutable std::vector<uint8_t> m_cacheData;
      Util::Mutex m_mutex;
    };
  }
}

#endif //_FABRIC_MR_VALUE_CACHE_H
