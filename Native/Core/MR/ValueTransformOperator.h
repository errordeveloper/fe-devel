/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_TRANSFORM_OPERATOR_H
#define _FABRIC_MR_VALUE_TRANSFORM_OPERATOR_H

#include <Fabric/Core/MR/ValueOutputOperator.h>

namespace Fabric
{
  namespace MR
  {
    class ValueTransformOperator : public ValueOutputOperator
    {
    public:
      REPORT_RC_LEAKS

      static RC::Handle<ValueTransformOperator> Create(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &valueDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
      
    protected:
    
      ValueTransformOperator();
    
    private:
    };
  }
}

#endif //_FABRIC_MR_VALUE_TRANSFORM_OPERATOR_H
