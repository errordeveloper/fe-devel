/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/FunctionWrapper.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/AST/Function.h>
#include <Fabric/Core/AST/GlobalList.h>

namespace Fabric
{
  namespace KLC
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( FunctionWrapper, GC::Object )
      
    FunctionWrapper::FunctionWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Function> const &astFunction,
      GenericFunctionPtr functionPtr
      )
      : GC::Object( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_executable( executable )
      , m_astFunction( astFunction )
      , m_functionPtr( functionPtr )
    {
    }

    RC::ConstHandle<AST::Function> FunctionWrapper::getASTFunction() const
    {
      return m_astFunction;
    }
    
    void FunctionWrapper::toJSON( JSON::Encoder &encoder ) const
    {
      if ( !m_astFunction || !m_functionPtr )
        throw Exception("function is unresolved");
    
      JSON::ObjectEncoder objectEncoder = encoder.makeObject();
      
      {
        objectEncoder.makeMember( "kind" ).makeString( getKind() );
      }
      
      toJSONImpl( objectEncoder );
    }
    
    void FunctionWrapper::toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const
    {
      objectEncoder.makeMember( "symbolName" ).makeString( m_astFunction->getSymbolName( m_executable->getCGManager() ) );
      
      {
        JSON::Encoder astEncoder = objectEncoder.makeMember( "ast" );
        getAST()->encodeJSON( false, astEncoder );
      }
    }
    
    RC::ConstHandle<AST::GlobalList> FunctionWrapper::getAST() const
    {
      return m_executable->getAST();
    }
    
    CG::Diagnostics const &FunctionWrapper::getDiagnostics() const
    {
      return m_executable->getDiagnostics();
    }
        
    void FunctionWrapper::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "getDiagnostics", 14 ) )
        jsonExecGetDiagnostics( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "toJSON", 6 ) )
        jsonExecToJSON( arg, resultArrayEncoder );
      else GC::Object::jsonExec( cmd, arg, resultArrayEncoder );
    }
    
    void FunctionWrapper::jsonExecGetDiagnostics(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      JSON::Encoder jg = resultArrayEncoder.makeElement();
      getDiagnostics().encodeJSON( jg );
    }
    
    void FunctionWrapper::jsonExecToJSON(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      JSON::Encoder jg = resultArrayEncoder.makeElement();
      toJSON( jg );
    }
  }
}
