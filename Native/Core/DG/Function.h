/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_FUNCTION_H
#define _FABRIC_DG_FUNCTION_H

#include <Fabric/Core/MT/Function.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace DG
  {
    class Code;
    class ExecutionEngine;
    
    class Function : public MT::Function
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<Function> Create( RC::ConstHandle<Code> const &code, std::string const &functionName );
    
      void onExecutionEngineChange( RC::ConstHandle<ExecutionEngine> const &executionEngine );
    
    protected:
    
      Function( RC::ConstHandle<Code> const &code, std::string const &functionName );
      ~Function();
      
    private:
    
      RC::ConstHandle<Code> m_code;
      std::string m_functionName;
    };
  };
};

#endif //_FABRIC_DG_FUNCTION_H
