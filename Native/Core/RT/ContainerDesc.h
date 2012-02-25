/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_CONTAINER_DESC_H
#define _FABRIC_RT_CONTAINER_DESC_H

#include <Fabric/Core/RT/Desc.h>

namespace Fabric
{
  namespace DG
  {
    class Container;
  }

  namespace RT
  {
    class ContainerImpl;
    
    class ContainerDesc : public Desc
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;

      void setValue( RC::Handle<DG::Container> const &container, void *data ) const;
      RC::Handle<DG::Container> getValue( void const *data ) const;

    protected:
    
      ContainerDesc( std::string const &name, RC::ConstHandle<ContainerImpl> const &containerImpl );
    
    private:
    
      RC::ConstHandle<ContainerImpl> m_containerImpl;
    };
  };
};

#endif //_FABRIC_RT_CONTAINER_DESC_H
