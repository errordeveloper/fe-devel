/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_RESOURCELOADNODE_H
#define _FABRIC_DG_RESOURCELOADNODE_H

#include <Fabric/Core/DG/Node.h>
#include <Fabric/Core/DG/FabricResource.h>
#include <Fabric/Core/IO/ResourceManager.h>
#include <fstream>

namespace Fabric
{
  namespace IO
  {
    class Stream;
    class ResourceManager;
  };

  namespace Util
  {
    class Timer;
  };

  namespace DG
  {
    class ResourceLoadNode : public Node, public IO::ResourceClient
    {

    public:
    
      static RC::Handle<ResourceLoadNode> Create( std::string const &name, RC::Handle<Context> const &context );
      static void jsonExecCreate( JSON::Entity const &arg, RC::Handle<Context> const &context, JSON::ArrayEncoder &resultArrayEncoder );

#if defined( FABRIC_RC_LEAK_REPORT )
      REPORT_RC_LEAKS
#else
      virtual void retain() const{ return Node::retain(); }
      virtual void release() const{ return Node::release(); }
#endif
    protected:
    
      ResourceLoadNode( std::string const &name, RC::Handle<Context> const &context );
      ~ResourceLoadNode();

      virtual void evaluateLocal( void *userdata );

      void setResourceData( char const *errorDesc, bool notify );

      void evaluateResource();
      static void EvaluateResource( void *userData, size_t index )
      {
        RC::Handle<ResourceLoadNode>::StaticCast(userData)->evaluateResource();
      }

      virtual void onProgress( char const *mimeType, size_t done, size_t total, void *userData );
      virtual void onData( size_t offset, size_t size, void const *data, void *userData );
      virtual void onFile( char const *fileName, void *userData );
      virtual void onFailure( char const *errorDesc, void *userData );

      void releaseFile();

    private:

      FabricResourceWrapper m_fabricResourceStreamData;
      bool m_firstEvalAfterLoad;
      bool m_keepMemoryCache;
      bool m_asFile;
      std::string m_file;
      bool m_inProgress;
      size_t m_streamGeneration;
      RC::WeakHandle<IO::ResourceManager> m_resourceManagerWeak;
    };
  };
};

#endif //_FABRIC_DG_RESOURCELOADNODE_H
