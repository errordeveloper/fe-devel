/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_MR_ARRAY_GENERATOR_OPERATOR_H
#define _FABRIC_MR_ARRAY_GENERATOR_OPERATOR_H

#include <Fabric/Core/MR/ArrayOutputOperator.h>

namespace Fabric
{
  namespace MR
  {
    class ArrayGeneratorOperator : public ArrayOutputOperator
    {
    public:
    
      static RC::Handle<ArrayGeneratorOperator> Create(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &valueDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
      
    protected:
    
      ArrayGeneratorOperator();
    };
  }
}

#endif //_FABRIC_MR_ARRAY_GENERATOR_OPERATOR_H
