/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ConstValue.h>
#include <Fabric/Core/MR/ConstValueWrapper.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ConstValueWrapper, ValueProducerWrapper );
    
    RC::Handle<ConstValueWrapper> ConstValueWrapper::Create(
      RC::ConstHandle<RT::Manager> const &rtManager,
      RC::ConstHandle<RT::Desc> const &valueDesc,
      JSON::Entity const &jsonEntity
      )
    {
      return new ConstValueWrapper( FABRIC_GC_OBJECT_MY_CLASS, rtManager, valueDesc, jsonEntity );
    }
    
    ConstValueWrapper::ConstValueWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<RT::Manager> const &rtManager,
      RC::ConstHandle<RT::Desc> const &valueDesc,
      JSON::Entity const &jsonEntity
      )
      : ValueProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_unwrapped(
        ConstValue::Create(
          rtManager,
          valueDesc,
          jsonEntity
        )
      )
    {
    }
      
    RC::ConstHandle<ValueProducer> ConstValueWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ConstValueWrapper::getKind() const
    {
      return "ConstValue";
    }
    
    void ConstValueWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
    {
      JSON::Encoder jg = objectEncoder.makeMember( "data" );
      m_unwrapped->getValueDesc()->encodeJSON( m_unwrapped->getImmutableData(), jg );
    }
  }
}
