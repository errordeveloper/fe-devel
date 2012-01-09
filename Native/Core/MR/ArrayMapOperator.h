/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_MR_ARRAY_MAP_OPERATOR_H
#define _FABRIC_MR_ARRAY_MAP_OPERATOR_H

#include <Fabric/Core/MR/ArrayIOOperator.h>

namespace Fabric
{
  namespace MR
  {
    class ArrayMapOperator : public ArrayIOOperator
    {
    public:
    
      static RC::Handle<ArrayMapOperator> Create(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &inputDesc,
        RC::ConstHandle<RT::Desc> const &outputDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
      
    protected:
    
      ArrayMapOperator();
    };
  }
}

#endif //_FABRIC_MR_ARRAY_MAP_OPERATOR_H
