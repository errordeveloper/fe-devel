/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_BOOLEAN_DESC_H
#define _FABRIC_RT_BOOLEAN_DESC_H

#include <Fabric/Core/RT/ComparableDesc.h>

namespace Fabric
{
  namespace RT
  {
    class BooleanImpl;
    
    class BooleanDesc : public ComparableDesc
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      bool getValue( void const *data ) const;
      void setValue( bool value, void *data ) const;
      
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      
    protected:
    
      BooleanDesc( std::string const &name, RC::ConstHandle<BooleanImpl> const &booleanImpl );
      
    private:
    
      RC::ConstHandle<BooleanImpl> m_booleanImpl;
    };
  };
};

#endif //_FABRIC_RT_BOOLEAN_DESC_H
