/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/KL/Externals.h>
#include <Fabric/Core/CG/ArrayProducerAdapter.h>
#include <Fabric/Core/CG/ArrayAdapter.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/CG/ValueProducerAdapter.h>
#include <Fabric/Core/MR/ArrayCache.h>
#include <Fabric/Core/MR/ArrayGenerator.h>
#include <Fabric/Core/MR/ArrayGeneratorOperator.h>
#include <Fabric/Core/MR/ArrayMap.h>
#include <Fabric/Core/MR/ArrayMapOperator.h>
#include <Fabric/Core/MR/ArrayTransform.h>
#include <Fabric/Core/MR/ArrayTransformOperator.h>
#include <Fabric/Core/MR/ConstArray.h>
#include <Fabric/Core/MR/ConstValue.h>
#include <Fabric/Core/MR/Reduce.h>
#include <Fabric/Core/MR/ReduceOperator.h>
#include <Fabric/Core/MR/ValueCache.h>
#include <Fabric/Core/MR/ValueGenerator.h>
#include <Fabric/Core/MR/ValueGeneratorOperator.h>
#include <Fabric/Core/MR/ValueMap.h>
#include <Fabric/Core/MR/ValueMapOperator.h>
#include <Fabric/Core/MR/ValueTransform.h>
#include <Fabric/Core/MR/ValueTransformOperator.h>
#include <Fabric/Core/RT/ArrayDesc.h>
#include <Fabric/Core/RT/ArrayProducerDesc.h>
#include <Fabric/Core/RT/ValueProducerDesc.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Base/Config.h>

#include <string>
#include <map>
#include <math.h>
#include <stdlib.h>

#if defined(FABRIC_OS_WINDOWS)
extern "C" void _chkstk();
    
static float imp_roundf( float x )
{
  return floorf( x + 0.5f );
}
    
static double imp_round( double x )
{
  return floor( x + 0.5 );
}
#endif

namespace Fabric
{
  static void MRCreateValueCache(
    MR::ValueProducer *&input,
    MR::ValueProducer const *&valueProducer
    )
  {
    if ( valueProducer )
      valueProducer->release();
    valueProducer = MR::ValueCache::Create(input).take();
  }

  static void MRCreateArrayCache(
    MR::ArrayProducer *&input,
    MR::ArrayProducer const *&arrayProducer
    )
  {
    if ( arrayProducer )
      arrayProducer->release();
    arrayProducer = MR::ArrayCache::Create(input).take();
  }

  static void MRCreateConstValue(
    CG::ValueProducerAdapter *valueProducerAdapter,
    void const *data,
    MR::ValueProducer const *&valueProducer
    )
  {
    if ( valueProducer )
      valueProducer->release();
    valueProducer = MR::ConstValue::Create(
      valueProducerAdapter->getRTManager(),
      RC::ConstHandle<RT::ValueProducerDesc>::StaticCast( valueProducerAdapter->getDesc() )->getValueDesc(),
      data
      ).take();
  }

  static void MRCreateValueTransform1(
    void (*operator_)(...),
    MR::ValueProducer *&input,
    MR::ValueProducer const *&valueProducer
    )
  {
    if ( valueProducer )
      valueProducer->release();
    valueProducer = MR::ValueTransform::Create(
      input,
      MR::ValueTransformOperator::Create(
        operator_,
        1,
        input->getValueDesc(),
        0
        ),
      0
      ).take();
  }

  static void MRCreateValueTransform2(
    void (*operator_)(...),
    MR::ValueProducer *&input,
    MR::ValueProducer *&shared,
    MR::ValueProducer const *&valueProducer
    )
  {
    if ( valueProducer )
      valueProducer->release();
    valueProducer = MR::ValueTransform::Create(
      input,
      MR::ValueTransformOperator::Create(
        operator_,
        2,
        input->getValueDesc(),
        shared->getValueDesc()
        ),
      shared
      ).take();
  }

  static void MRCreateValueMap2(
    void (*operator_)(...),
    MR::ValueProducer *&input,
    CG::ValueProducerAdapter *outputValueProducerAdapter,
    MR::ValueProducer const *&valueProducer
    )
  {
    if ( valueProducer )
      valueProducer->release();
    valueProducer = MR::ValueMap::Create(
      input,
      MR::ValueMapOperator::Create(
        operator_,
        2,
        input->getValueDesc(),
        RC::ConstHandle<RT::ValueProducerDesc>::StaticCast( outputValueProducerAdapter->getDesc() )->getValueDesc(),
        0
        ),
      0
      ).take();
  }

  static void MRCreateValueMap3(
    void (*operator_)(...),
    MR::ValueProducer *&input,
    CG::ValueProducerAdapter *outputValueProducerAdapter,
    MR::ValueProducer *&shared,
    MR::ValueProducer const *&valueProducer
    )
  {
    if ( valueProducer )
      valueProducer->release();
    valueProducer = MR::ValueMap::Create(
      input,
      MR::ValueMapOperator::Create(
        operator_,
        3,
        input->getValueDesc(),
        RC::ConstHandle<RT::ValueProducerDesc>::StaticCast( outputValueProducerAdapter->getDesc() )->getValueDesc(),
        shared->getValueDesc()
        ),
      shared
      ).take();
  }

  static void MRCreateArrayGenerator3(
    void (*operator_)(...),
    size_t numParams,
    MR::ValueProducer *&countValueProducer,
    CG::ArrayProducerAdapter *outputArrayProducerAdapter,
    MR::ArrayProducer const *&outputArrayProducer
    )
  {
    if ( outputArrayProducer )
      outputArrayProducer->release();
    outputArrayProducer = MR::ArrayGenerator::Create(
      countValueProducer,
      MR::ArrayGeneratorOperator::Create(
        operator_,
        numParams,
        RC::ConstHandle<RT::ArrayProducerDesc>::StaticCast( outputArrayProducerAdapter->getDesc() )->getElementDesc(),
        0
        ),
      0
      ).take();
  }

  static void MRCreateArrayGenerator4(
    void (*operator_)(...),
    size_t numParams,
    MR::ValueProducer *&countValueProducer,
    MR::ValueProducer *&sharedValueProducer,
    CG::ArrayProducerAdapter *outputArrayProducerAdapter,
    MR::ArrayProducer const *&outputArrayProducer
    )
  {
    if ( outputArrayProducer )
      outputArrayProducer->release();
    outputArrayProducer = MR::ArrayGenerator::Create(
      countValueProducer,
      MR::ArrayGeneratorOperator::Create(
        operator_,
        numParams,
        RC::ConstHandle<RT::ArrayProducerDesc>::StaticCast( outputArrayProducerAdapter->getDesc() )->getElementDesc(),
        sharedValueProducer->getValueDesc()
        ),
      sharedValueProducer
      ).take();
  }

  static void MRCreateValueGenerator1(
    void (*operator_)(...),
    size_t numParams,
    CG::ValueProducerAdapter *outputValueProducerAdapter,
    MR::ValueProducer const *&outputValueProducer
    )
  {
    if ( outputValueProducer )
      outputValueProducer->release();
    outputValueProducer = MR::ValueGenerator::Create(
      MR::ValueGeneratorOperator::Create(
        operator_,
        numParams,
        RC::ConstHandle<RT::ValueProducerDesc>::StaticCast( outputValueProducerAdapter->getDesc() )->getValueDesc(),
        0
        ),
      0
      ).take();
  }

  static void MRCreateValueGenerator2(
    void (*operator_)(...),
    size_t numParams,
    MR::ValueProducer *&sharedValueProducer,
    CG::ValueProducerAdapter *outputValueProducerAdapter,
    MR::ValueProducer const *&outputValueProducer
    )
  {
    if ( outputValueProducer )
      outputValueProducer->release();
    outputValueProducer = MR::ValueGenerator::Create(
      MR::ValueGeneratorOperator::Create(
        operator_,
        numParams,
        RC::ConstHandle<RT::ValueProducerDesc>::StaticCast( outputValueProducerAdapter->getDesc() )->getValueDesc(),
        sharedValueProducer->getValueDesc()
        ),
      sharedValueProducer
      ).take();
  }

  static void MRCreateConstArray(
    CG::ArrayAdapter *arrayAdapter,
    void const *arrayData,
    CG::ArrayProducerAdapter *arrayProducerAdapter,
    MR::ArrayProducer const *&arrayProducer
    )
  {
    if ( arrayProducer )
      arrayProducer->release();
    arrayProducer = MR::ConstArray::Create(
      arrayProducerAdapter->getRTManager(),
      RC::ConstHandle<RT::ArrayDesc>::StaticCast( arrayAdapter->getDesc() ),
      arrayData
      ).take();
  }

  static void MRCreateArrayMap4(
    void (*operator_)(...),
    size_t numParams,
    MR::ArrayProducer *&inputArrayProducer,
    CG::ArrayProducerAdapter *outputArrayProducerAdapter,
    MR::ArrayProducer const *&arrayProducer
    )
  {
    if ( arrayProducer )
      arrayProducer->release();
    arrayProducer = MR::ArrayMap::Create(
      inputArrayProducer,
      MR::ArrayMapOperator::Create(
        operator_,
        numParams,
        inputArrayProducer->getElementDesc(),
        RC::ConstHandle<RT::ArrayProducerDesc>::StaticCast( outputArrayProducerAdapter->getDesc() )->getElementDesc(),
        0
        ),
      0
      ).take();
  }

  static void MRCreateArrayMap5(
    void (*operator_)(...),
    size_t numParams,
    MR::ArrayProducer *&inputArrayProducer,
    CG::ArrayProducerAdapter *outputArrayProducerAdapter,
    MR::ValueProducer *&sharedValueProducer,
    MR::ArrayProducer const *&arrayProducer
    )
  {
    if ( arrayProducer )
      arrayProducer->release();
    arrayProducer = MR::ArrayMap::Create(
      inputArrayProducer,
      MR::ArrayMapOperator::Create(
        operator_,
        numParams,
        inputArrayProducer->getElementDesc(),
        RC::ConstHandle<RT::ArrayProducerDesc>::StaticCast( outputArrayProducerAdapter->getDesc() )->getElementDesc(),
        sharedValueProducer->getValueDesc()
        ),
      sharedValueProducer
      ).take();
  }

  static void MRCreateArrayTransform3(
    void (*operator_)(...),
    size_t numParams,
    MR::ArrayProducer *&ioArrayProducer,
    MR::ArrayProducer const *&arrayProducer
    )
  {
    if ( arrayProducer )
      arrayProducer->release();
    arrayProducer = MR::ArrayTransform::Create(
      ioArrayProducer,
      MR::ArrayTransformOperator::Create(
        operator_,
        numParams,
        ioArrayProducer->getElementDesc(),
        0
        ),
      0
      ).take();
  }

  static void MRCreateArrayTransform4	(
    void (*operator_)(...),
    size_t numParams,
    MR::ArrayProducer *&ioArrayProducer,
    MR::ValueProducer *&sharedValueProducer,
    MR::ArrayProducer const *&arrayProducer
    )
  {
    if ( arrayProducer )
      arrayProducer->release();
    arrayProducer = MR::ArrayTransform::Create(
      ioArrayProducer,
      MR::ArrayTransformOperator::Create(
        operator_,
        numParams,
        ioArrayProducer->getElementDesc(),
        sharedValueProducer->getValueDesc()
        ),
      sharedValueProducer
      ).take();
  }

  static void MRCreateReduce4(
    void (*operator_)(...),
    size_t numParams,
    MR::ArrayProducer *&inputArrayProducer,
    CG::ValueProducerAdapter *outputValueProducerAdapter,
    MR::ValueProducer const *&valueProducer
    )
  {
    if ( valueProducer )
      valueProducer->release();
    valueProducer = MR::Reduce::Create(
      inputArrayProducer,
      MR::ReduceOperator::Create(
        operator_,
        numParams,
        inputArrayProducer->getElementDesc(),
        RC::ConstHandle<RT::ValueProducerDesc>::StaticCast( outputValueProducerAdapter->getDesc() )->getValueDesc(),
        0
        ),
      0
      ).take();
  }

  static void MRCreateReduce5(
    void (*operator_)(...),
    size_t numParams,
    MR::ArrayProducer *&inputArrayProducer,
    CG::ValueProducerAdapter *outputValueProducerAdapter,
    MR::ValueProducer *&sharedValueProducer,
    MR::ValueProducer const *&valueProducer
    )
  {
    if ( valueProducer )
      valueProducer->release();
    valueProducer = MR::Reduce::Create(
      inputArrayProducer,
      MR::ReduceOperator::Create(
        operator_,
        numParams,
        inputArrayProducer->getElementDesc(),
        RC::ConstHandle<RT::ValueProducerDesc>::StaticCast( outputValueProducerAdapter->getDesc() )->getValueDesc(),
        sharedValueProducer->getValueDesc()
        ),
      sharedValueProducer
      ).take();
  }

  namespace KL
  {
    class SymbolNameToAddressMap : public std::map<std::string, void *>
    {
    public:
    
      SymbolNameToAddressMap()
      {
        SymbolNameToAddressMap &symbolNameToAddressMap = *this;
        
        // Internal: allocation
        symbolNameToAddressMap["malloc"] = (void *)&malloc;
        symbolNameToAddressMap["realloc"] = (void *)&realloc;
        symbolNameToAddressMap["free"] = (void *)&free;

        // LLVM intrinsics
        symbolNameToAddressMap["sinf"] = (void *)&sinf;
        symbolNameToAddressMap["cosf"] = (void *)&cosf;
        symbolNameToAddressMap["fmodf"] = (void *)&fmodf;
#if defined(FABRIC_OS_WINDOWS)
        symbolNameToAddressMap["sin"] = (void *)(double (*)(double))&sin;
        symbolNameToAddressMap["cos"] = (void *)(double (*)(double))&cos;
        symbolNameToAddressMap["fmod"] = (void *)(double (*)(double,double))&fmod;
#else
        symbolNameToAddressMap["sin"] = (void *)&sin;
        symbolNameToAddressMap["cos"] = (void *)&cos;
        symbolNameToAddressMap["fmod"] = (void *)&fmod;
#endif

        // Exposed math functions
        symbolNameToAddressMap["__function_acos__in_Scalar"] = (void *)&acosf;
        symbolNameToAddressMap["__function_asin__in_Scalar"] = (void *)&asinf;
        symbolNameToAddressMap["__function_atan__in_Scalar"] = (void *)&atanf;
        symbolNameToAddressMap["__function_atan2__in_Scalar__in_Scalar"] = (void *)&atan2f;
        symbolNameToAddressMap["__function_sin__in_Scalar"] = (void *)&sinf;
        symbolNameToAddressMap["__function_cos__in_Scalar"] = (void *)&cosf;
        symbolNameToAddressMap["__function_tan__in_Scalar"] = (void *)&tanf;
#if defined(FABRIC_OS_WINDOWS)
        symbolNameToAddressMap["__function_acos__in_Float64"] = (void *)(double (*)(double))&acos;
        symbolNameToAddressMap["__function_asin__in_Float64"] = (void *)(double (*)(double))&asin;
        symbolNameToAddressMap["__function_atan__in_Float64"] = (void *)(double (*)(double))&atan;
        symbolNameToAddressMap["__function_atan2__in_Float64__in_Float64"] = (void *)(double (*)(double,double))&atan2;
        symbolNameToAddressMap["__function_sin__in_Float64"] = (void *)(double (*)(double))&sin;
        symbolNameToAddressMap["__function_cos__in_Float64"] = (void *)(double (*)(double))&cos;
        symbolNameToAddressMap["__function_tan__in_Float64"] = (void *)(double (*)(double))&tan;
#else
        symbolNameToAddressMap["__function_acos__in_Float64"] = (void *)&acos;
        symbolNameToAddressMap["__function_asin__in_Float64"] = (void *)&asin;
        symbolNameToAddressMap["__function_atan__in_Float64"] = (void *)&atan;
        symbolNameToAddressMap["__function_atan2__in_Float64__in_Float64"] = (void *)&atan2;
        symbolNameToAddressMap["__function_sin__in_Float64"] = (void *)&sin;
        symbolNameToAddressMap["__function_cos__in_Float64"] = (void *)&cos;
        symbolNameToAddressMap["__function_tan__in_Float64"] = (void *)&tan;
#endif

#if defined(FABRIC_OS_WINDOWS)
        symbolNameToAddressMap["__function_round__in_Scalar"] = (void *)&imp_roundf;
        symbolNameToAddressMap["__function_round__in_Float64"] = (void *)&imp_round;
#else
        symbolNameToAddressMap["__function_round__in_Scalar"] = (void *)&roundf;
        symbolNameToAddressMap["__function_round__in_Float64"] = (void *)&round;
#endif
        symbolNameToAddressMap["__function_ceil__in_Scalar"] =( void *)&ceilf;
        symbolNameToAddressMap["__function_floor__in_Scalar"] =( void *)&floorf;
        symbolNameToAddressMap["__function_pow__in_Scalar__in_Scalar"] = (void *)&powf;
        symbolNameToAddressMap["__function_log__in_Scalar"] = (void *)&logf;
        symbolNameToAddressMap["__function_log10__in_Scalar"] = (void *)&log10f;
        symbolNameToAddressMap["__function_exp__in_Scalar"] = (void *)&expf;

#if defined(FABRIC_OS_WINDOWS)
        symbolNameToAddressMap["__function_ceil__in_Float64"] =( void *)(double (*)(double))&ceil;
        symbolNameToAddressMap["__function_floor__in_Float64"] =( void *)(double (*)(double))&floor;
        symbolNameToAddressMap["__function_pow__in_Float64__in_Float64"] = (void *)(double (*)(double,double))&pow;
        symbolNameToAddressMap["__function_log10__in_Float64"] = (void *)(double (*)(double))&log10;
        symbolNameToAddressMap["__function_log__in_Float64"] = (void *)(double (*)(double))&log;
        symbolNameToAddressMap["__function_exp__in_Float64"] = (void *)(double (*)(double))&exp;
#else
        symbolNameToAddressMap["__function_ceil__in_Float64"] =( void *)&ceil;
        symbolNameToAddressMap["__function_floor__in_Float64"] =( void *)&floor;
        symbolNameToAddressMap["__function_pow__in_Float64__in_Float64"] = (void *)&pow;
        symbolNameToAddressMap["__function_log10__in_Float64"] = (void *)&log10;
        symbolNameToAddressMap["__function_log__in_Float64"] = (void *)&log;
        symbolNameToAddressMap["__function_exp__in_Float64"] = (void *)&exp;
#endif

        // Weird Windows stuff
#if defined(FABRIC_OS_WINDOWS)
        symbolNameToAddressMap["_chkstk"] = (void *)&_chkstk;
#endif

        // Map-reduce
        symbolNameToAddressMap["__MR_CreateValueCache"] = (void *)&MRCreateValueCache;
        symbolNameToAddressMap["__MR_CreateArrayCache"] = (void *)&MRCreateArrayCache;
        symbolNameToAddressMap["__MR_CreateConstValue"] = (void *)&MRCreateConstValue;
        symbolNameToAddressMap["__MR_CreateValueGenerator_1"] = (void *)&MRCreateValueGenerator1;
        symbolNameToAddressMap["__MR_CreateValueGenerator_2"] = (void *)&MRCreateValueGenerator2;
        symbolNameToAddressMap["__MR_CreateValueMap_2"] = (void *)&MRCreateValueMap2;
        symbolNameToAddressMap["__MR_CreateValueMap_3"] = (void *)&MRCreateValueMap3;
        symbolNameToAddressMap["__MR_CreateValueTransform_1"] = (void *)&MRCreateValueTransform1;
        symbolNameToAddressMap["__MR_CreateValueTransform_2"] = (void *)&MRCreateValueTransform2;
        symbolNameToAddressMap["__MR_CreateConstArray"] = (void *)&MRCreateConstArray;
        symbolNameToAddressMap["__MR_CreateArrayGenerator_3"] = (void *)&MRCreateArrayGenerator3;
        symbolNameToAddressMap["__MR_CreateArrayGenerator_4"] = (void *)&MRCreateArrayGenerator4;
        symbolNameToAddressMap["__MR_CreateArrayMap_4"] = (void *)&MRCreateArrayMap4;
        symbolNameToAddressMap["__MR_CreateArrayMap_5"] = (void *)&MRCreateArrayMap5;
        symbolNameToAddressMap["__MR_CreateArrayTransform_3"] = (void *)&MRCreateArrayTransform3;
        symbolNameToAddressMap["__MR_CreateArrayTransform_4"] = (void *)&MRCreateArrayTransform4;
        symbolNameToAddressMap["__MR_CreateReduce_4"] = (void *)&MRCreateReduce4;
        symbolNameToAddressMap["__MR_CreateReduce_5"] = (void *)&MRCreateReduce5;
      }
    };

    static SymbolNameToAddressMap s_symbolNameToAddressMap;

    void *LookupExternalSymbol( std::string const &functionName )
    {
      SymbolNameToAddressMap::const_iterator it = s_symbolNameToAddressMap.find( functionName );
      if ( it != s_symbolNameToAddressMap.end() )
        return it->second;
      else return 0;
    }
  }
}
