/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_CONST_STRING_DESC_H
#define _FABRIC_RT_CONST_STRING_DESC_H

#include <Fabric/Core/RT/Desc.h>

namespace Fabric
{
  namespace RT
  {
    class ConstStringImpl;
    
    class ConstStringDesc : public Desc
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
      
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
            
      char const *getValueData( void const *src ) const;
      size_t getValueLength( void const *src ) const;

      std::string toString( void const *data ) const;
      
    protected:
    
      ConstStringDesc( std::string const &name, RC::ConstHandle<ConstStringImpl> const &constStringImpl );
      
    private:
    
      RC::ConstHandle<ConstStringImpl> m_constStringImpl;
   };
  };
};

#endif //_FABRIC_RT_CONST_STRING_DESC_H
