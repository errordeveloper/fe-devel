/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_NUMERIC_DESC_H
#define _FABRIC_RT_NUMERIC_DESC_H

#include <Fabric/Core/RT/ComparableDesc.h>
#include <Fabric/Core/RT/NumericImpl.h>

namespace Fabric
{
  namespace RT
  {
    class NumericDesc : public ComparableDesc
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      bool isInteger() const
      {
        return m_numericImpl->isInteger();
      }
      bool isFloat() const
      {
        return m_numericImpl->isFloat();
      }
      
    protected:
    
      NumericDesc( std::string const &name, RC::ConstHandle<NumericImpl> const &numericImpl )
        : ComparableDesc( name, numericImpl )
        , m_numericImpl( numericImpl )
      {
      }
      
    private:
    
      RC::ConstHandle<NumericImpl> m_numericImpl;
    };
  };
};

#endif //_FABRIC_RT_NUMERIC_DESC_H
