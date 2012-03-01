/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_METHOD_BUILDER_H
#define _FABRIC_CG_METHOD_BUILDER_H

#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/Mangling.h>

namespace Fabric
{
  namespace CG
  {
    class MethodBuilder : public FunctionBuilder
    {
    public:
    
      MethodBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &returnAdapter,
        RC::ConstHandle<Adapter> const &thisAdapter,
        Usage thisUsage,
        std::string const &methodName
        )
        : FunctionBuilder(
          moduleBuilder,
          MethodPencilKey(
            thisAdapter,
            methodName
            ),
          MethodDefaultSymbolName(
            ExprType( thisAdapter, thisUsage ),
            methodName,
            ExprTypeVector()
            ),
          MethodFullDesc(
            returnAdapter,
            ExprType( thisAdapter, thisUsage ),
            methodName,
            ExprTypeVector()
            ),
          returnAdapter,
          ParamVector(
            FriendlyFunctionParam( "this", thisAdapter, thisUsage )
            ),
          0
          )
      {
      }
    
      MethodBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &returnAdapter,
        RC::ConstHandle<Adapter> const &thisAdapter,
        Usage thisUsage,
        std::string const &methodName,
        std::string const &paramName,
        RC::ConstHandle<Adapter> const &paramAdapter,
        Usage paramUsage
        )
        : FunctionBuilder(
          moduleBuilder,
          MethodPencilKey(
            thisAdapter,
            methodName
            ),
          MethodDefaultSymbolName(
            ExprType( thisAdapter, thisUsage ),
            methodName,
            ExprTypeVector(
              ExprType( paramAdapter, paramUsage )
              )
            ),
          MethodFullDesc(
            returnAdapter,
            ExprType( thisAdapter, thisUsage ),
            methodName,
            ExprTypeVector(
              ExprType( paramAdapter, paramUsage )
              )
            ),
          returnAdapter,
          ParamVector(
            FriendlyFunctionParam( "this", thisAdapter, thisUsage ),
            FriendlyFunctionParam( paramName, paramAdapter, paramUsage )
            ),
          0
          )
      {
      }
    
      MethodBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &returnAdapter,
        RC::ConstHandle<Adapter> const &thisAdapter,
        Usage thisUsage,
        std::string const &methodName,
        std::string const &param1Name,
        RC::ConstHandle<Adapter> const &param1Adapter,
        Usage param1Usage,
        std::string const &param2Name,
        RC::ConstHandle<Adapter> const &param2Adapter,
        Usage param2Usage
        )
        : FunctionBuilder(
          moduleBuilder,
          MethodPencilKey(
            thisAdapter,
            methodName
            ),
          MethodDefaultSymbolName(
            ExprType( thisAdapter, thisUsage ),
            methodName,
            ExprTypeVector(
              ExprType( param1Adapter, param1Usage ),
              ExprType( param2Adapter, param2Usage )
              )
            ),
          MethodFullDesc(
            returnAdapter,
            ExprType( thisAdapter, thisUsage ),
            methodName,
            ExprTypeVector(
              ExprType( param1Adapter, param1Usage ),
              ExprType( param2Adapter, param2Usage )
              )
            ),
          returnAdapter,
          ParamVector(
            FriendlyFunctionParam( "this", thisAdapter, thisUsage ),
            FriendlyFunctionParam( param1Name, param1Adapter, param1Usage ),
            FriendlyFunctionParam( param2Name, param2Adapter, param2Usage )
            ),
          0
          )
      {
      }
    };
  }
}

#endif //_FABRIC_CG_METHOD_BUILDER_H
