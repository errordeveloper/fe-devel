/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_FUNCTION_WRAPPER_H
#define _FABRIC_KLC_FUNCTION_WRAPPER_H

#include <Fabric/Core/GC/Object.h>
#include <Fabric/Core/KLC/GenericFunctionPtr.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
    class ObjectEncoder;
  };
  
  namespace AST
  {
    class Function;
    class GlobalList;
  };
  
  namespace CG
  {
    class Diagnostics;
  };
  
  namespace KLC
  {
    class Executable;
    
    class FunctionWrapper : public GC::Object
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
    public:
      REPORT_RC_LEAKS
    
      RC::ConstHandle<AST::GlobalList> getAST() const;
      CG::Diagnostics const &getDiagnostics() const;
      RC::ConstHandle<AST::Function> getASTFunction() const;
      
      void toJSON( JSON::Encoder &jg ) const;
        
      virtual void jsonExec(
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
    protected:
    
      FunctionWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Function> const &astFunction,
        GenericFunctionPtr functionPtr
        );
       
      virtual char const *getKind() const = 0;
      virtual void toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const;
   
    private:
    
      void jsonExecGetDiagnostics(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
    
      void jsonExecToJSON(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
    
      RC::ConstHandle<Executable> m_executable;
      RC::ConstHandle<AST::Function> m_astFunction;
      GenericFunctionPtr m_functionPtr;
    };
  }
}

#endif //_FABRIC_KLC_FUNCTION_WRAPPER_H
