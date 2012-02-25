/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_REDUCE_OPERATOR_H
#define _FABRIC_MR_REDUCE_OPERATOR_H

#include <Fabric/Core/MR/ArrayIOOperator.h>

namespace Fabric
{
  namespace MR
  {
    class ReduceOperator : public ArrayIOOperator
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ReduceOperator> Create(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &inputDesc,
        RC::ConstHandle<RT::Desc> const &outputDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
      
    protected:
    
      ReduceOperator();
    };
  }
}

#endif //_FABRIC_MR_REDUCE_OPERATOR_H
