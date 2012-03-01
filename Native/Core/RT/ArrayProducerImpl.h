/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_ARRAY_PRODUCER_IMPL_H
#define _FABRIC_RT_ARRAY_PRODUCER_IMPL_H

#include <Fabric/Core/RT/ProducerImpl.h>

namespace Fabric
{
  namespace JSON
  {
    struct Entity;
  }
  
  namespace MR
  {
    class ArrayProducer;
  }
  
  namespace RT
  {
    class ArrayProducerImpl : public ProducerImpl
    {
      friend class Impl;
      friend class Manager;

    public:
      REPORT_RC_LEAKS
    
      // Impl
      
      virtual void setData( void const *value, void *data ) const;
      virtual void disposeDatasImpl( void *data, size_t count, size_t stride ) const;
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      virtual bool equalsData( void const *lhs, void const *rhs ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;

      virtual bool isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const;
      virtual bool isShallow() const;
      virtual bool isNoAliasSafe() const;
    
      // ArrayProducerImpl
      
      RC::ConstHandle<Impl> getElementImpl() const;

    protected:
      
      ArrayProducerImpl( std::string const &codeName, RC::ConstHandle<RT::Impl> const &valueImpl );
    
    private:
    
      RC::ConstHandle<Impl> m_elementImpl;
    };
  }
}

#endif //_FABRIC_RT_ARRAY_PRODUCER_IMPL_H
