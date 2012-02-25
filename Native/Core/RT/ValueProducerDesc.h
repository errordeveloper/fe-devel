/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_VALUE_PRODUCER_DESC_H
#define _FABRIC_RT_VALUE_PRODUCER_DESC_H

#include <Fabric/Core/RT/Desc.h>

namespace Fabric
{
  namespace RT
  {
    class ValueProducerImpl;
    
    class ValueProducerDesc : public Desc
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      
      RC::ConstHandle<Desc> getValueDesc() const;
      
    protected:
    
      ValueProducerDesc( std::string const &name, RC::ConstHandle<ValueProducerImpl> const &valueProducerImpl, RC::ConstHandle<RT::Desc> const &valueDesc );
    
    private:
    
      RC::ConstHandle<ValueProducerImpl> m_valueProducerImpl;
      RC::ConstHandle<Desc> m_valueDesc;
    };
  };
};

#endif //_FABRIC_RT_VALUE_PRODUCER_DESC_H
