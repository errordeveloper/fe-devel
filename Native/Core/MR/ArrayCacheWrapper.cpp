/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayCacheWrapper.h>
#include <Fabric/Core/MR/ArrayCache.h>
#include <Fabric/Core/MR/ArrayProducerWrapper.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ArrayCacheWrapper, ArrayProducerWrapper );
    
    RC::Handle<ArrayCacheWrapper> ArrayCacheWrapper::Create(
      RC::ConstHandle<ArrayProducerWrapper> const &inputArrayProducer
      )
    {
      return new ArrayCacheWrapper( FABRIC_GC_OBJECT_MY_CLASS, inputArrayProducer );
    }
    
    ArrayCacheWrapper::ArrayCacheWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<ArrayProducerWrapper> const &inputArrayProducer
      )
      : ArrayProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_inputArrayProducer( inputArrayProducer )
      , m_unwrapped(
        ArrayCache::Create(
          inputArrayProducer->getUnwrapped()
          )
        )
    {
    }
      
    RC::ConstHandle<ArrayProducer> ArrayCacheWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ArrayCacheWrapper::getKind() const
    {
      return "ArrayCache";
    }
    
    void ArrayCacheWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
    {
      {
        JSON::Encoder inputEncoder = objectEncoder.makeMember( "input" );
        m_inputArrayProducer->toJSON( inputEncoder );
      }
    }
  }
}
