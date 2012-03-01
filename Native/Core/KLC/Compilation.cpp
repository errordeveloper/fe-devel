/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/KLC/Compilation.h>
#include <Fabric/Core/KL/Scanner.h>
#include <Fabric/Core/KL/StringSource.h>
#include <Fabric/Core/CG/Manager.h>

namespace Fabric
{
  namespace KLC
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( Compilation, GC::Object )
      
    RC::Handle<Compilation> Compilation::Create(
      GC::Container *gcContainer,
      RC::Handle<CG::Manager> const &cgManager,
      CG::CompileOptions const &compileOptions
      )
    {
      return new Compilation(
        FABRIC_GC_OBJECT_MY_CLASS,
        gcContainer,
        cgManager,
        compileOptions
        );
    }

    Compilation::Compilation(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      GC::Container *gcContainer,
      RC::Handle<CG::Manager> const &cgManager,
      CG::CompileOptions const &compileOptions
      )
      : GC::Object( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_gcContainer( gcContainer )
      , m_cgManager( cgManager )
      , m_compileOptions( compileOptions )
    {
    }

    Compilation::~Compilation()
    {
    }
    
    void Compilation::add( std::string const &sourceName, std::string const &sourceCode )
    {
      Source &source = m_sources[sourceName];
      if ( source.sourceCode != sourceCode )
      {
        source.sourceCode = sourceCode;
        source.diagnostics.clear();
        source.ast = 0;
      }
    }
    
    void Compilation::remove( std::string const &sourceName )
    {
      SourceMap::iterator it = m_sources.find( sourceName );
      if ( it != m_sources.end() )
        m_sources.erase( it );
    }
    
    void Compilation::compileAll() const
    {
      for ( SourceMap::iterator it = m_sources.begin(); it != m_sources.end(); ++it )
      {
        std::string const &sourceName = it->first;
        Source &source = it->second;
        if ( source.diagnostics.empty() && !source.ast )
        {
          RC::ConstHandle<KL::Source> stringSource = KL::StringSource::Create( sourceName, source.sourceCode );
          RC::Handle<KL::Scanner> scanner = KL::Scanner::Create( stringSource );
          source.ast = KL::Parse( scanner, source.diagnostics );
        }
      }
    }
    
    RC::Handle<Executable> Compilation::run() const
    {
      compileAll();
      
      RC::ConstHandle<AST::GlobalList> globalAST;
      CG::Diagnostics diagnostics;
      for ( SourceMap::const_iterator it = m_sources.begin(); it != m_sources.end(); ++it )
      {
        Source const &source = it->second;
        globalAST = AST::GlobalList::Create( globalAST, source.ast );
        diagnostics.append( source.diagnostics );
      }
        
      return Executable::Create( m_gcContainer, m_cgManager, globalAST, m_compileOptions, diagnostics );
    }
        
    void Compilation::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "addSource", 9 ) )
        jsonExecAddSource( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "removeSource", 12 ) )
        jsonExecRemoveSource( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getSources", 10 ) )
        jsonExecGetSources( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "run", 3 ) )
        jsonExecRun( arg, resultArrayEncoder );
      else GC::Object::jsonExec( cmd, arg, resultArrayEncoder );
    }
    
    void Compilation::jsonExecAddSource(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string sourceName;
      std::string sourceCode;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "sourceName", 10 ) )
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

      if ( sourceName.empty() )
        throw Exception( "missing 'sourceName'" );
      if ( sourceCode.empty() )
        throw Exception( "missing 'sourceCode'" );
      
      add( sourceName, sourceCode );
    }
    
    void Compilation::jsonExecRemoveSource(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string sourceName;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "sourceName", 10 ) )
          {
            valueEntity.requireString();
            sourceName = valueEntity.stringToStdString();
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }

      if ( sourceName.empty() )
        throw Exception( "missing 'sourceName'" );
      
      remove( sourceName );
    }
    
    void Compilation::jsonExecGetSources(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      for ( SourceMap::const_iterator it = m_sources.begin(); it != m_sources.end(); ++it )
      {
        resultObjectEncoder.makeMember( it->first ).makeString( it->second.sourceCode );
      }
    }
    
    void Compilation::jsonExecRun(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;

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
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }

      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      
      run()->reg( m_gcContainer, id_ );
    }
  }
}
