/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_STRING_DESC_H
#define _FABRIC_RT_STRING_DESC_H

#include <Fabric/Core/RT/ComparableDesc.h>

namespace Fabric
{
  namespace RT
  {
    class StringImpl;
    
    class StringDesc : public ComparableDesc
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      char const *getValueData( void const *src ) const;
      size_t getValueLength( void const *src ) const;
      void setValue( char const *data, size_t length, void *dst ) const;
      
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      
    protected:
    
      StringDesc( std::string const &name, RC::ConstHandle<StringImpl> const &stringImpl );
    
    private:
    
      RC::ConstHandle<StringImpl> m_stringImpl;
    };
  };
};

#endif //_FABRIC_RT_STRING_DESC_H
