/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ValueIOOperator.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/AST/Operator.h>
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Core/RT/IntegerDesc.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace KLC
  {
    ValueIOOperator::ValueIOOperator(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      GenericFunctionPtr functionPtr
      )
      : m_executable( executable )
    {
      if ( astOperator )
      {
        RC::Handle<CG::Manager> cgManager = executable->getCGManager();
        
        RC::ConstHandle<AST::ParamVector> params = astOperator->getParams( cgManager );
        size_t numParams = params->size();
        if ( numParams < 2 || numParams > 3 )
          throw GetPrototypeException();
        
        RC::ConstHandle<AST::Param> inputParam = params->get(0);
        if ( inputParam->getUsage() != CG::USAGE_RVALUE )
          throw GetPrototypeException();
        RC::ConstHandle<RT::Desc> inputDesc = inputParam->getAdapter( cgManager )->getDesc();
        
        RC::ConstHandle<AST::Param> outputParam = params->get(1);
        if ( outputParam->getUsage() != CG::USAGE_LVALUE )
          throw GetPrototypeException();
        RC::ConstHandle<RT::Desc> outputDesc = outputParam->getAdapter( cgManager )->getDesc();
        
        RC::ConstHandle<RT::Desc> sharedDesc;
        if ( numParams >= 3 )
        {
          RC::ConstHandle<AST::Param> sharedParam = params->get(2);
          if ( sharedParam->getUsage() != CG::USAGE_RVALUE )
            throw GetPrototypeException();
          sharedDesc = sharedParam->getAdapter( cgManager )->getDesc();
        }

        init( functionPtr, numParams, inputDesc, outputDesc, sharedDesc );
      }
    }
  }
}
