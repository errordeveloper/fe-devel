/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_CG_UNI_OP_BUILDER_H
#define _FABRIC_CG_UNI_OP_BUILDER_H

#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/Mangling.h>

namespace Fabric
{
  namespace CG
  {
    class UniOpBuilder : public FunctionBuilder
    {
    public:
    
      UniOpBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &resultAdapter,
        UniOpType type,
        RC::ConstHandle<Adapter> const &thisAdapter,
        Usage thisUsage
        )
        : FunctionBuilder(
          moduleBuilder,
          UniOpPencilKey(
            type
            ),
          UniOpDefaultSymbolName(
            type,
            ExprType( thisAdapter, thisUsage )
            ),
          UniOpFullDesc(
            resultAdapter,
            type,
            ExprType( thisAdapter, thisUsage )
            ),
          resultAdapter,
          ParamVector(
            FriendlyFunctionParam( "this", thisAdapter, thisUsage )
            ),
          0
          )
      {
      }
    };
  }
}

#endif //_FABRIC_CG_UNI_OP_BUILDER_H
