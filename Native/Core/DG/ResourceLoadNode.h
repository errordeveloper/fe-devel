/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_DG_RESOURCELOADNODE_H
#define _FABRIC_DG_RESOURCELOADNODE_H

#include <Fabric/Core/DG/Node.h>
#include <Fabric/Core/DG/FabricResource.h>
#include <Fabric/Core/IO/ResourceManager.h>

namespace Fabric
{
  namespace IO
  {
    class Stream;
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
      static void jsonExecCreate( RC::ConstHandle<JSON::Value> const &arg, RC::Handle<Context> const &context, Util::JSONArrayGenerator &resultJAG );

      virtual void retain() const;
      virtual void release() const;

    protected:
    
      ResourceLoadNode( std::string const &name, RC::Handle<Context> const &context );

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

    private:

      FabricResourceWrapper m_fabricResourceStreamData;
      bool m_firstEvalAfterLoad;
      bool m_keepMemoryCache;
      bool m_asFile;
      bool m_inProgress;
      size_t m_streamGeneration;
    };
  };
};

#endif //_FABRIC_DG_RESOURCELOADNODE_H
