/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ArrayOutputOperator.h>
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
    ArrayOutputOperator::ArrayOutputOperator(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      void (*functionPtr)(...)
      )
      : m_executable( executable )
    {
      if ( astOperator )
      {
        RC::Handle<CG::Manager> cgManager = executable->getCGManager();
        
        RC::ConstHandle<AST::ParamVector> params = astOperator->getParams( cgManager );
        size_t numParams = params->size();
        if ( numParams < 1 || numParams > 4 )
          throw GetPrototypeException();
        
        RC::ConstHandle<AST::Param> valueParam = params->get(0);
        if ( valueParam->getUsage() != CG::USAGE_LVALUE )
          throw GetPrototypeException();
        RC::ConstHandle<RT::Desc> valueDesc = valueParam->getAdapter( cgManager )->getDesc();
        
        RC::ConstHandle<RT::Desc> sharedDesc;
        if ( numParams >= 2 )
        {
          RC::ConstHandle<AST::Param> indexParam = params->get(1);
          if ( indexParam->getUsage() != CG::USAGE_RVALUE
            || indexParam->getAdapter( cgManager ) != cgManager->getSizeAdapter()
            )
            throw GetPrototypeException();
        
          if ( numParams >= 3 )
          {
            RC::ConstHandle<AST::Param> countParam = params->get(2);
            if ( countParam->getUsage() != CG::USAGE_RVALUE
              || countParam->getAdapter( cgManager ) != cgManager->getSizeAdapter()
              )
              throw GetPrototypeException();
          
            if ( numParams >= 4 )
            {
              RC::ConstHandle<AST::Param> sharedParam = params->get(3);
              if ( sharedParam->getUsage() != CG::USAGE_RVALUE )
                throw GetPrototypeException();
              sharedDesc = sharedParam->getAdapter( cgManager )->getDesc();
            }
          }
        }

        init( functionPtr, numParams, valueDesc, sharedDesc );
      }
    }
  }
}
