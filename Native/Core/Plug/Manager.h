/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_PLUG_MANAGER_H
#define _FABRIC_PLUG_MANAGER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/Plug/Helpers.h>

#include <map>
 
namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace RT
  {
    class Impl;
    class Manager;
  };
  
  namespace EDK
  {
    struct Callbacks;
  };

  namespace CG
  {
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class GlobalList;
  };
  
  namespace DG
  {
    class Context;
  };
  
  namespace IO
  {
    class Dir;
  };
  
  namespace Plug
  {
    class Inst;
    
    class Manager : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      typedef std::map< std::string, RC::Handle<Inst> > NameToInstMap;
    
      static RC::Handle<Manager> Instance();
      static void Terminate();
      
      void loadBuiltInPlugins( std::vector<std::string> const &pluginDirs, RC::Handle<CG::Manager> const &cgManager, EDK::Callbacks const &callbacks );
      
      NameToInstMap const &getRegisteredPlugins() const
      {
        return m_nameToInstMap;
      }
      
      RC::ConstHandle<AST::GlobalList> getAST() const;
      RC::ConstHandle<AST::GlobalList> maybeGetASTForExt( std::string const &extName ) const;
      void *llvmResolveExternalFunction( std::string const &name ) const;

      virtual void jsonDesc( JSON::Encoder &resultEncoder ) const;

    protected:
    
      Manager();
      ~Manager();
      
      RC::ConstHandle<Inst> registerPlugin( RC::ConstHandle<IO::Dir> const &extensionDir, std::string const &name, std::string const &jsonDesc, std::vector<std::string> const &pluginDirs, RC::Handle<CG::Manager> const &cgManager, EDK::Callbacks const &callbacks );
      
    private:
    
      bool m_loaded;
      std::vector<std::string> m_pluginDirs;
      std::string m_fabricSDKSOLibResolvedName;
      //SOLibHandle m_fabricSDKSOLibHandle;
      NameToInstMap m_nameToInstMap;
      std::map< std::string, void (*)( void * ) > m_implNameToDestructorMap;
    };
  };
};

#endif //_FABRIC_PLUG_MANAGER_H
