/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_CG_BIN_OP_BUILDER_H
#define _FABRIC_CG_BIN_OP_BUILDER_H

#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/Mangling.h>

namespace Fabric
{
  namespace CG
  {
    class BinOpBuilder : public FunctionBuilder
    {
    public:
    
      BinOpBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &resultAdapter,
        BinOpType type,
        RC::ConstHandle<Adapter> const &lhsAdapter,
        RC::ConstHandle<Adapter> const &rhsAdapter
        )
        : FunctionBuilder(
          moduleBuilder,
          BinOpPencilKey(
            type
            ),
          BinOpDefaultSymbolName(
            type,
            lhsAdapter,
            rhsAdapter
            ),
          BinOpFullDesc(
            resultAdapter,
            type,
            lhsAdapter,
            rhsAdapter
            ),
          resultAdapter,
          ParamVector(
            FriendlyFunctionParam( "lhs", lhsAdapter, USAGE_RVALUE ),
            FriendlyFunctionParam( "rhs", rhsAdapter, USAGE_RVALUE )
            ),
          0
          )
      {
      }
    };
  }
}

#endif //_FABRIC_CG_BIN_OP_BUILDER_H
