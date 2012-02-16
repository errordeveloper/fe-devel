/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_AST_PARAM_VECTOR_H
#define _FABRIC_AST_PARAM_VECTOR_H

#include <Fabric/Core/CG/FunctionParam.h>
#include <Fabric/Base/RC/Vector.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace llvm
{
  class Type;
};

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace CG
  {
    class Adapter;
    class Diagnostics;
    class ExprType;
    class Manager;
    class ModuleBuilder;
  };

  namespace AST
  {
    class Param;
    
    class ParamVector : public RC::Vector< RC::ConstHandle<Param> >
    {
    public:
      
      static RC::ConstHandle<ParamVector> Create( RC::ConstHandle<Param> const &firstParam = 0, RC::ConstHandle<ParamVector> const &remainingParams = 0 );
      static RC::ConstHandle<ParamVector> Create( RC::ConstHandle<Param> const &firstParam, RC::ConstHandle<Param> const &secondParam );

      void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
    
      CG::ParamVector getFunctionParams( RC::Handle<CG::Manager> const &cgManager ) const;
      std::vector<std::string> getTypes() const;
      CG::AdapterVector getAdapters( RC::Handle<CG::Manager> const &cgManager ) const;
      CG::ExprTypeVector getExprTypes( RC::Handle<CG::Manager> const &cgManager ) const;
      
      void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const;
      
    protected:
    
      ParamVector();
    };
  };
};

#endif //_FABRIC_AST_PARAM_VECTOR_H
