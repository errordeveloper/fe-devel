/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ValueProducerDesc.h"
#include "ValueProducerImpl.h"
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    ValueProducerDesc::ValueProducerDesc(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<ValueProducerImpl> const &valueProducerImpl,
      RC::ConstHandle<RT::Desc> const &valueDesc
      )
      : Desc(
        userNameBase,
        userNameArraySuffix,
        valueProducerImpl
        )
      , m_valueProducerImpl( valueProducerImpl )
      , m_valueDesc( valueDesc )
    {
    }
    
    void ValueProducerDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Desc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "internalType" ).makeString( "valueProducer" );
    }
    
    RC::ConstHandle<Desc> ValueProducerDesc::getValueDesc() const
    {
      return m_valueDesc;
    }
  }
}
