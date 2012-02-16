/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_CG_INTERNAL_FUNCTION_BUILDER_H
#define _FABRIC_CG_INTERNAL_FUNCTION_BUILDER_H

#include <Fabric/Core/CG/FunctionBuilder.h>

namespace Fabric
{
  namespace CG
  {
    class InternalFunctionBuilder : public FunctionBuilder
    {
    public:
    
      InternalFunctionBuilder(
        ModuleBuilder &moduleBuilder, 
        RC::ConstHandle<CG::Adapter> const &returnAdapter,
        std::string const &internalFunctionName,
        std::string const &param1Name,
        RC::ConstHandle<CG::Adapter> const &param1Adapter,
        Usage param1Usage,
        size_t flags
        )
        : FunctionBuilder(
          moduleBuilder,
          "",
          internalFunctionName,
          "internal function " + internalFunctionName,
          returnAdapter,
          ParamVector(
            FriendlyFunctionParam( param1Name, param1Adapter, param1Usage )
            ),
          flags
          )
      {
      }
          
      InternalFunctionBuilder(
        ModuleBuilder &moduleBuilder, 
        RC::ConstHandle<CG::Adapter> const &returnAdapter,
        std::string const &internalFunctionName,
        std::string const &param1Name,
        RC::ConstHandle<CG::Adapter> const &param1Adapter,
        Usage param1Usage,
        std::string const &param2Name,
        RC::ConstHandle<CG::Adapter> const &param2Adapter,
        Usage param2Usage,
        size_t flags
        )
        : FunctionBuilder(
          moduleBuilder,
          "",
          internalFunctionName,
          "internal function " + internalFunctionName,
          returnAdapter,
          ParamVector(
            FriendlyFunctionParam( param1Name, param1Adapter, param1Usage ),
            FriendlyFunctionParam( param2Name, param2Adapter, param2Usage )
            ),
          flags
          )
      {
      }
          
      InternalFunctionBuilder(
        ModuleBuilder &moduleBuilder, 
        RC::ConstHandle<CG::Adapter> const &returnAdapter,
        std::string const &internalFunctionName,
        ParamVector const &params,
        size_t flags
        )
        : FunctionBuilder(
          moduleBuilder,
          "",
          internalFunctionName,
          "internal function " + internalFunctionName,
          returnAdapter,
          params,
          flags
          )
      {
      }
    };
  }
}

#endif //_FABRIC_CG_INTERNAL_FUNCTION_BUILDER_H
