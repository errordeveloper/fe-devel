/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef __FABRIC_AST_OPERATOR_H__
#define __FABRIC_AST_OPERATOR_H__

#include <Fabric/Core/AST/Function.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace AST
  {
    class ParamVector;
    
    class Operator : public Function
    {
      FABRIC_AST_NODE_DECL( Operator );

    public:
    
      static RC::ConstHandle<Function> Create(
        CG::Location const &location,
        std::string const &functionName,
        RC::ConstHandle<ParamVector> const &params,
        std::string const *symbolName,
        RC::ConstHandle<CompoundStatement> const &body
        );

      virtual bool isOperator() const { return true; }

    protected:
    
      Operator( 
        CG::Location const &location,
        std::string const &functionName,
        RC::ConstHandle<ParamVector> const &params,
        std::string const *symbolName,
        RC::ConstHandle<CompoundStatement> const &body
        );
    };
  }
}

#endif // __FABRIC_AST_OPERATOR_H__
