/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ConstArrayWrapper.h>
#include <Fabric/Core/MR/ConstArray.h>
#include <Fabric/Core/RT/ArrayDesc.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ConstArrayWrapper, ArrayProducerWrapper );
    
    RC::Handle<ConstArrayWrapper> ConstArrayWrapper::Create(
      RC::ConstHandle<RT::Manager> const &rtManager,
      RC::ConstHandle<RT::Desc> const &elementDesc,
      JSON::Entity const &entity
      )
    {
      return new ConstArrayWrapper( FABRIC_GC_OBJECT_MY_CLASS, rtManager, elementDesc, entity );
    }
    
    ConstArrayWrapper::ConstArrayWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<RT::Manager> const &rtManager,
      RC::ConstHandle<RT::Desc> const &elementDesc,
      JSON::Entity const &entity
      )
      : ArrayProducerWrapper( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_unwrapped( ConstArray::Create( rtManager, elementDesc, entity ) )
    {
    }
      
    RC::ConstHandle<ArrayProducer> ConstArrayWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }

    char const *ConstArrayWrapper::getKind() const
    {
      return "ConstArray";
    }
    
    void ConstArrayWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
    {
      {
        JSON::Encoder jg = objectEncoder.makeMember( "elementType" );
        jg.makeString( m_unwrapped->getElementDesc()->getUserName() );
      }
      
      {
        JSON::Encoder jg = objectEncoder.makeMember( "data" );
        m_unwrapped->getArrayDesc()->encodeJSON( m_unwrapped->getImmutableData(), jg );
      }
    }
  }
}
