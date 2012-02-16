/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_CG_CONSTRUCTOR_BUILDER_H
#define _FABRIC_CG_CONSTRUCTOR_BUILDER_H

#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/Mangling.h>

namespace Fabric
{
  namespace CG
  {
    class ConstructorBuilder : public FunctionBuilder
    {
    public:
    
      enum Cost
      {
        LowCost = 10,
        MedLowCost = 100,
        MedHighCost = 1000,
        HighCost = 10000
      };
    
      ConstructorBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &thisAdapter,
        RC::ConstHandle<Adapter> const &thatAdapter,
        Cost cost = ConstructorBuilder::HighCost
        )
        : FunctionBuilder(
          moduleBuilder,
          ConstructorPencilKey( thisAdapter ),
          ConstructorDefaultSymbolName(
            thisAdapter,
            AdapterVector(
              thatAdapter
              )
            ),
          ConstructorFullDesc(
            thisAdapter,
            AdapterVector(
              thatAdapter
              )
            ),
          0,
          ParamVector(
            FunctionParam( "thisLValue", thisAdapter, USAGE_LVALUE ),
            FunctionParam( "thatRValue", thatAdapter, USAGE_RVALUE )
            ),
          0,
          size_t( cost )
          )
      {
      }
    
      ConstructorBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &thisAdapter,
        std::string const &param1Name, RC::ConstHandle<Adapter> const &param1Adapter
        )
        : FunctionBuilder(
          moduleBuilder,
          ConstructorPencilKey( thisAdapter ),
          ConstructorDefaultSymbolName(
            thisAdapter,
            AdapterVector(
              param1Adapter
              )
            ),
          ConstructorFullDesc(
            thisAdapter,
            AdapterVector(
              param1Adapter
              )
            ),
          0,
          ParamVector(
            FunctionParam( "thisLValue", thisAdapter, USAGE_LVALUE ),
            FriendlyFunctionParam( param1Name, param1Adapter, USAGE_RVALUE )
            ),
          0
          )
      {
      }
    
      ConstructorBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &thisAdapter,
        std::string const &param1Name, RC::ConstHandle<Adapter> const &param1Adapter,
        std::string const &param2Name, RC::ConstHandle<Adapter> const &param2Adapter
        )
        : FunctionBuilder(
          moduleBuilder,
          ConstructorPencilKey( thisAdapter ),
          ConstructorDefaultSymbolName(
            thisAdapter,
            AdapterVector(
              param1Adapter,
              param2Adapter
              )
            ),
          ConstructorFullDesc(
            thisAdapter,
            AdapterVector(
              param1Adapter,
              param2Adapter
              )
            ),
          0,
          ParamVector(
            FunctionParam( "thisLValue", thisAdapter, USAGE_LVALUE ),
            FriendlyFunctionParam( param1Name, param1Adapter, USAGE_RVALUE ),
            FriendlyFunctionParam( param2Name, param2Adapter, USAGE_RVALUE )
            ),
          0
          )
      {
      }
    
      ConstructorBuilder( 
        ModuleBuilder &moduleBuilder,
        RC::ConstHandle<Adapter> const &thisAdapter,
        std::string const &param1Name, RC::ConstHandle<Adapter> const &param1Adapter,
        std::string const &param2Name, RC::ConstHandle<Adapter> const &param2Adapter,
        std::string const &param3Name, RC::ConstHandle<Adapter> const &param3Adapter
        )
        : FunctionBuilder(
          moduleBuilder,
          ConstructorPencilKey( thisAdapter ),
          ConstructorDefaultSymbolName(
            thisAdapter,
            AdapterVector(
              param1Adapter,
              param2Adapter,
              param3Adapter
              )
            ),
          ConstructorFullDesc(
            thisAdapter,
            AdapterVector(
              param1Adapter,
              param2Adapter,
              param3Adapter
              )
            ),
          0,
          ParamVector(
            FunctionParam( "thisLValue", thisAdapter, USAGE_LVALUE ),
            FriendlyFunctionParam( param1Name, param1Adapter, USAGE_RVALUE ),
            FriendlyFunctionParam( param2Name, param2Adapter, USAGE_RVALUE ),
            FriendlyFunctionParam( param3Name, param3Adapter, USAGE_RVALUE )
            ),
          0
          )
      {
      }
    };
  }
}

#endif //_FABRIC_CG_CONSTRUCTOR_BUILDER_H
