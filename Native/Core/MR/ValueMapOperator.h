/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_MR_VALUE_MAP_OPERATOR_H
#define _FABRIC_MR_VALUE_MAP_OPERATOR_H

#include <Fabric/Core/MR/ValueIOOperator.h>

namespace Fabric
{
  namespace MR
  {
    class ValueMapOperator : public ValueIOOperator
    {
    public:
    
      static RC::Handle<ValueMapOperator> Create(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &inputDesc,
        RC::ConstHandle<RT::Desc> const &outputDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
      
    protected:
    
      ValueMapOperator();
    };
  }
}

#endif //_FABRIC_MR_VALUE_MAP_OPERATOR_H
