/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "ContainerDesc.h"
#include "ContainerImpl.h"
#include <Fabric/Core/DG/Container.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace RT
  {
    ContainerDesc::ContainerDesc(
      std::string const &name,
      RC::ConstHandle<ContainerImpl> const &containerImpl
      )
      : Desc( name, containerImpl )
      , m_containerImpl( containerImpl )
    {
    }
    
    void ContainerDesc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      Desc::jsonDesc( resultObjectEncoder );
      resultObjectEncoder.makeMember( "internalType" ).makeString( "Container" );
    }

    void ContainerDesc::setValue( RC::Handle<DG::Container> const &container, void *data ) const
    {
      m_containerImpl->setValue( container, data );
    }

    RC::Handle<DG::Container> ContainerDesc::getValue( void const *data ) const
    {
      return m_containerImpl->getValue( data );
    }
  }
}
