/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_MR_ARRAY_TRANSFORM_OPERATOR_H
#define _FABRIC_MR_ARRAY_TRANSFORM_OPERATOR_H

#include <Fabric/Core/MR/ArrayOutputOperator.h>

namespace Fabric
{
  namespace MR
  {
    class ArrayTransformOperator : public ArrayOutputOperator
    {
    public:
    
      static RC::Handle<ArrayTransformOperator> Create(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &valueDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
      
    protected:
    
      ArrayTransformOperator();
    };
  }
}

#endif //_FABRIC_MR_ARRAY_TRANSFORM_OPERATOR_H
