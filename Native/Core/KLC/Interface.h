/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_INTERFACE_H
#define _FABRIC_KLC_INTERFACE_H

#include <Fabric/Core/KLC/Compilation.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/GC/Object.h>
#include <Fabric/Core/GC/Container.h>
#include <Fabric/Core/CG/CompileOptions.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Base/JSON/Decoder.h>

namespace Fabric
{
  namespace JSON
  {
    class ArrayEncoder;
  };
  
  namespace KLC
  {
    class Interface
    {
    public:
    
      Interface(
        GC::Container *gcContainer,
        RC::Handle<CG::Manager> const &cgManager,
        CG::CompileOptions const &compileOptions
        );
    
      void jsonRoute(
        std::vector<JSON::Entity> const &dst,
        size_t dstOffset,
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
        
      void jsonExec(
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateCompilation(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateExecutable(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      template<class T> void jsonExecCreateOperator(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        )
      {
        std::string id_;
        std::string sourceName;
        std::string sourceCode;
        std::string operatorName;
        
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
            else if ( keyString.stringIs( "operatorName", 12 ) )
            {
              valueEntity.requireString();
              operatorName = valueEntity.stringToStdString();
            }
          }
          catch ( Exception e )
          {
            argObjectDecoder.rethrow( e );
          }
        }
        
        RC::Handle<Compilation> compilation = Compilation::Create( m_gcContainer, m_cgManager, m_compileOptions );
        compilation->add( sourceName, sourceCode );
        RC::Handle<Executable> executable = compilation->run();
        executable->resolveOperator<T>( operatorName )->reg( m_gcContainer, id_ );
      }
        
    private:
    
      GC::Container *m_gcContainer;
      RC::Handle<CG::Manager> m_cgManager;
      CG::CompileOptions m_compileOptions;
    };
  };
};

#endif //_FABRIC_KLC_INTERFACE_H
