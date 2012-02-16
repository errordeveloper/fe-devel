/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_CG_ASS_OP_BUILDER_H
#define _FABRIC_CG_ASS_OP_BUILDER_H

#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/Mangling.h>

namespace Fabric
{
  namespace CG
  {
    class AssOpBuilder : public FunctionBuilder
    {
    public:
    
      AssOpBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &thisAdapter,
        AssignOpType type,
        RC::ConstHandle<Adapter> const &thatAdapter
        )
        : FunctionBuilder(
          moduleBuilder,
          AssignOpPencilKey(
            thisAdapter,
            type
            ),
          AssignOpDefaultSymbolName(
            thisAdapter,
            type,
            thatAdapter
            ),
          AssignOpFullDesc(
            thisAdapter,
            type,
            thatAdapter
            ),
          0,
          ParamVector(
            FriendlyFunctionParam( "this", thisAdapter, USAGE_LVALUE ),
            FriendlyFunctionParam( "that", thatAdapter, USAGE_RVALUE )
            ),
          0
          )
      {
      }
    };
  }
}

#endif //_FABRIC_CG_ASS_OP_BUILDER_H
