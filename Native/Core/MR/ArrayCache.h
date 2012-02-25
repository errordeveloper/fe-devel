/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_ARRAY_CACHE_H
#define _FABRIC_MR_ARRAY_CACHE_H

#include <Fabric/Core/MR/ArrayProducer.h>
#include <Fabric/Core/MR/ValueProducer.h>
#include <Fabric/Core/Util/Mutex.h>

#include <stdint.h>
#include <vector>

namespace Fabric
{
  namespace MR
  {
    class ArrayOutputOperator;
    
    class ArrayCache : public ArrayProducer
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ArrayCache> Create(
        RC::ConstHandle<ArrayProducer> const &inputArrayProducer
        );
      
      // Virtual functions: ArrayProducer
    
    public:
      
      virtual RC::ConstHandle<RT::Desc> getElementDesc() const;
      virtual const RC::Handle<ArrayProducer::ComputeState> createComputeState() const;
      virtual void flush();
            
    protected:
    
      class ComputeState : public ArrayProducer::ComputeState
      {
      public:
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<ArrayCache> const &arrayCache );
      
        virtual void produce( size_t index, void *data ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<ArrayCache> const &arrayCache );
        
      private:
      
        RC::ConstHandle<ArrayCache> m_arrayCache;
        RC::ConstHandle<ArrayProducer::ComputeState> m_inputArrayProducerComputeState;
        mutable Util::Mutex m_mutex;
      };
    
      ArrayCache(
        RC::ConstHandle<ArrayProducer> const &inputArrayProducer
        );
      ~ArrayCache();
    
    private:
    
      RC::ConstHandle<ArrayProducer> m_inputArrayProducer;

      RC::ConstHandle<RT::Desc> m_inputElementDesc;
      
      mutable size_t m_cacheCount;
      mutable bool m_cacheCountExists;
      mutable std::vector<bool> m_cacheDataExists;
      mutable std::vector<uint8_t> m_cacheData;
      mutable Util::Mutex m_mutex;
    };
  };
};

#endif //_FABRIC_MR_ARRAY_CACHE_H
