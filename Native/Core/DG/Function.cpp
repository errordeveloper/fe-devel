/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/Function.h>
#include <Fabric/Core/DG/Code.h>
#include <Fabric/Core/DG/ExecutionEngine.h>
#include <Fabric/Core/MT/ParallelCall.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace DG
  {
    RC::ConstHandle<Function> Function::Create( RC::ConstHandle<Code> const &code, std::string const &functionName )
    {
      return new DG::Function( code, functionName );
    }
    
    Function::Function( RC::ConstHandle<Code> const &code, std::string const &functionName )
      : m_code( code )
      , m_functionName( functionName )
    {
      m_code->registerFunction( this );
      onExecutionEngineChange( code->getExecutionEngine() );
    }
    
    Function::~Function()
    {
      m_code->unregisterFunction( this );
    }

    void Function::onExecutionEngineChange( RC::ConstHandle<ExecutionEngine> const &executionEngine )
    {
      FunctionPtr functionPtr = executionEngine->getFunctionByName( m_functionName );
      if ( !functionPtr )
        throw Exception( "function " + _(m_functionName) + " not found" );
      onFunctionPtrChange( functionPtr, executionEngine );
    }
  };
};
