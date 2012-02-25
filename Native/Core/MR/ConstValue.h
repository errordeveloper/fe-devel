/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_CONST_VALUE_H
#define _FABRIC_MR_CONST_VALUE_H

#include <Fabric/Core/MR/ValueProducer.h>

#include <stdint.h>
#include <vector>

namespace Fabric
{
  namespace JSON
  {
    struct Entity;
  }
  
  namespace RT
  {
    class Manager;
  }
  
  namespace MR
  {
    class ConstValue : public ValueProducer
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ConstValue> Create(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &valueDesc,
        JSON::Entity const &entity
        );
      static RC::Handle<ConstValue> Create(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &valueDesc,
        void const *data
        );
      
      // Virtual functions: ValueProducer
    
    public:
      
      virtual RC::ConstHandle<RT::Desc> getValueDesc() const;
      virtual const RC::Handle<ValueProducer::ComputeState> createComputeState() const;
      
      virtual void const *getImmutableData() const;
      virtual void flush();
            
    protected:
    
      class ComputeState : public ValueProducer::ComputeState
      {
      public:
      
        static RC::Handle<ComputeState> Create( RC::ConstHandle<ConstValue> const &valueMap );
     
        virtual void produce( void *data ) const;
        virtual void produceJSON( JSON::Encoder &jg ) const;
      
      protected:
      
        ComputeState( RC::ConstHandle<ConstValue> const &valueMap );
        ~ComputeState();
        
      private:
      
        RC::ConstHandle<ConstValue> m_constValue;
      };
      
      ConstValue(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &valueDesc,
        JSON::Entity const &entity
        );
      ConstValue(
        RC::ConstHandle<RT::Manager> const &rtManager,
        RC::ConstHandle<RT::Desc> const &valueDesc,
        void const *data
        );
      ~ConstValue();
    
    private:
    
      RC::ConstHandle<RT::Desc> m_valueDesc;
      std::vector<uint8_t> m_data;
    };
  }
}

#endif //_FABRIC_MR_CONST_VALUE_H
