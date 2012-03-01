/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_CONTEXT_H
#define _FABRIC_NPAPI_CONTEXT_H

#include <Fabric/Core/DG/Context.h>

#include <vector>
#include <map>

namespace Fabric
{
  namespace JSON
  {
    class ArrayEncoder;
  };
  
  namespace LIB
  {
    class Value;
    class Callback;
  };

  namespace IO
  {
    class Dir;
  };
  
  namespace NPAPI
  {
    class IOManager;
    class ViewPort;
    
    class Context : public DG::Context
    {
      typedef std::map< std::string, ViewPort * > ViewPorts;
      
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<Context> Create( RC::Handle<IOManager> const &ioManager, std::vector<std::string> const &pluginDirs );
      
      void registerViewPort( std::string const &name, ViewPort *viewPort );
      void unregisterViewPort( std::string const &name, ViewPort *viewPort );
      
      virtual void jsonRoute( std::vector<JSON::Entity> const &dst, size_t dstOffset, JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      virtual void jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      virtual void jsonDesc( JSON::Encoder &resultEncoder ) const;
      
      void jsonRouteViewPorts( std::vector<JSON::Entity> const &dst, size_t dstOffset, JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonDescViewPorts( JSON::Encoder &resultEncoder ) const;

      RC::Handle<IOManager> getIOManager() const;

      std::string queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension ) const;

    protected:
    
      Context( RC::Handle<IOManager> const &ioManager, std::vector<std::string> const &pluginDirs );
      ~Context();
      
    private:
      
      RC::Handle<IOManager> m_ioManager;
      ViewPorts m_viewPorts;
    };
  };
};

#endif
