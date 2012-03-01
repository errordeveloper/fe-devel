/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_CONST_ARRAY_PRODUCER_H
#define _FABRIC_MR_CONST_ARRAY_PRODUCER_H

#include <Fabric/Core/MR/ArrayProducer.h>

#include <vector>
#include <stdint.h>

namespace Fabric
{
  namespace JSON
  {
    struct Entity;
  };
  
  namespace RT
  {
    class ArrayDesc;
    class FixedArrayDesc;
    class Manager;
  };
  
  namespace MR
  {
    class ConstArray : public ArrayProducer
    {
    public:
      REPORT_RC_LEAKS

      static RC::Handle<ConstArray> Create(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &elementDesc,
        JSON::Entity const &entity
        );

      static RC::Handle<ConstArray> Create(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::ArrayDesc> const &arrayDesc,
        void const *data
        );
      
      // Virtual functions: ArrayProducer
    
    public:
      
      virtual RC::ConstHandle<RT::Desc> getElementDesc() const;
      virtual const RC::Handle<ArrayProducer::ComputeState> createComputeState() const;

      RC::ConstHandle<RT::ArrayDesc> getArrayDesc() const;
      void const *getImmutableData() const;
      virtual void flush();
      
    protected:
    
      class ComputeState : public ArrayProducer::ComputeState
      {
      public:
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<ConstArray> const &constArray );
      
        virtual void produce( size_t index, void *data ) const;
        virtual void produceJSON( size_t index, JSON::Encoder &jg ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<ConstArray> const &constArray );
        
      private:
      
        RC::ConstHandle<ConstArray> m_constArray;
      };
        
      ConstArray(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &elementDesc,
        JSON::Entity const &entity
        );

      ConstArray(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::ArrayDesc> const &arrayDesc,
        void const *data
        );
      
      ~ConstArray();
    
    private:
    
      RC::ConstHandle<RT::FixedArrayDesc> m_fixedArrayDesc;
      std::vector<uint8_t> m_data;
    };
  };
};

#endif //_FABRIC_MR_CONST_ARRAY_PRODUCER_H
