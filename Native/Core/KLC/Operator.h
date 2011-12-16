/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_KL_OPERATOR_H
#define _FABRIC_KL_OPERATOR_H

#include <Fabric/Core/KLC/Function.h>

namespace Fabric
{
  namespace AST
  {
    class Operator;
  };
  
  namespace KLC
  {
    class Operator : public Function
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
    public:
    
      static RC::Handle<Operator> Create(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
      
      virtual char const *getKind() const;

    protected:
    
      Operator(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
    };
  }
}

#endif //_FABRIC_KL_OPERATOR_H
