/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/Interface.h>
#include <Fabric/Core/KLC/ArrayGeneratorOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayMapOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayTransformOperatorWrapper.h>
#include <Fabric/Core/KLC/Compilation.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/KLC/ReduceOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueGeneratorOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueMapOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueTransformOperatorWrapper.h>
#include <Fabric/Core/GC/Object.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Core/Util/Parse.h>

namespace Fabric
{
  namespace KLC
  {
    Interface::Interface(
      GC::Container *gcContainer,
      RC::Handle<CG::Manager> const &cgManager,
      CG::CompileOptions const &compileOptions
      )
      : m_gcContainer( gcContainer )
      , m_cgManager( cgManager )
      , m_compileOptions( compileOptions )
    {
    }
  
    void Interface::jsonRoute(
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( dst.size() - dstOffset == 0 )
        jsonExec( cmd, arg, resultArrayEncoder );
      else m_gcContainer->jsonRoute( dst, dstOffset, cmd, arg, resultArrayEncoder );
    }
      
    void Interface::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "createCompilation", 17 ) )
        jsonExecCreateCompilation( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createExecutable", 16 ) )
        jsonExecCreateExecutable( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createValueMapOperator", 22 ) )
        jsonExecCreateOperator<ValueMapOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createValueGeneratorOperator", 28 ) )
        jsonExecCreateOperator<ValueGeneratorOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createArrayGeneratorOperator", 28 ) )
        jsonExecCreateOperator<ArrayGeneratorOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createValueTransformOperator", 28 ) )
        jsonExecCreateOperator<ValueTransformOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createArrayMapOperator", 22 ) )
        jsonExecCreateOperator<ArrayMapOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createArrayTransformOperator", 28 ) )
        jsonExecCreateOperator<ArrayTransformOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createReduceOperator", 20 ) )
        jsonExecCreateOperator<ReduceOperatorWrapper>( arg, resultArrayEncoder );
      else throw Exception( "unknown command: " + _(cmd) );
    }
    
    void Interface::jsonExecCreateCompilation(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      std::string sourceName;
      std::string sourceCode;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "sourceName", 10 ) )
          {
            valueEntity.requireString();
            sourceName = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "sourceCode", 10 ) )
          {
            valueEntity.requireString();
            sourceCode = valueEntity.stringToStdString();
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      
      RC::Handle<Compilation> compilation = Compilation::Create( m_gcContainer, m_cgManager, m_compileOptions );
      if ( sourceName.length() > 0 || sourceCode.length() > 0 )
        compilation->add( sourceName, sourceCode );
      compilation->reg( m_gcContainer, id_ );
    }
    
    void Interface::jsonExecCreateExecutable(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      std::string sourceName;
      std::string sourceCode;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "sourceName", 10 ) )
          {
            valueEntity.requireString();
            sourceName = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "sourceCode", 10 ) )
          {
            valueEntity.requireString();
            sourceCode = valueEntity.stringToStdString();
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( sourceName.empty() )
        throw Exception( "missing 'sourceName'" );
      if ( sourceCode.empty() )
        throw Exception( "missing 'sourceCode'" );
      
      RC::Handle<Compilation> compilation = Compilation::Create( m_gcContainer, m_cgManager, m_compileOptions );
      compilation->add( sourceName, sourceCode );
      compilation->run()->reg( m_gcContainer, id_ );
    }
  };
};
