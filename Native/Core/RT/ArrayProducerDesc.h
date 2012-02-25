/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_ARRAY_PRODUCER_DESC_H
#define _FABRIC_RT_ARRAY_PRODUCER_DESC_H

#include <Fabric/Core/RT/Desc.h>

namespace Fabric
{
  namespace RT
  {
    class ArrayProducerImpl;
    
    class ArrayProducerDesc : public Desc
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      
      RC::ConstHandle<Desc> getElementDesc() const;
      
    protected:
    
      ArrayProducerDesc( std::string const &name, RC::ConstHandle<ArrayProducerImpl> const &arrayProducerImpl, RC::ConstHandle<RT::Desc> const &elementDesc );
    
    private:
    
      RC::ConstHandle<ArrayProducerImpl> m_arrayProducerImpl;
      RC::ConstHandle<Desc> m_elementDesc;
    };
  };
};

#endif //_FABRIC_RT_ARRAY_PRODUCER_DESC_H
