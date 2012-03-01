/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/IRCache.h>
#include <Fabric/Core/IO/Dir.h>
#include <Fabric/Core/Util/MD5.h>
#include <Fabric/Core/AST/GlobalList.h>
#include <Fabric/Core/CG/CompileOptions.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Core/Build.h>

#include <map>

#define FABRIC_IR_CACHE_EXPIRY_SEC (30*24*60*60)

namespace Fabric
{
  namespace DG
  {
    std::map< std::string, RC::Handle<IRCache> > g_instances;

    RC::Handle<IRCache> IRCache::Instance( CG::CompileOptions const *compileOptions )
    {
      std::string compileOptionsString = compileOptions->getString();
      RC::Handle<IRCache> &instance = g_instances[compileOptionsString];
      if ( !instance )
        instance = new IRCache( compileOptionsString );

      return instance;
    }

    void IRCache::Terminate()
    {
      g_instances.clear();
    }
      
    IRCache::IRCache( std::string const &compileOptionsString )
    {
      RC::ConstHandle<IO::Dir> rootDir = IO::Dir::Private();
      RC::ConstHandle<IO::Dir> baseDir = IO::Dir::Create( rootDir, "IRCache" );
      baseDir->recursiveDeleteFilesOlderThan( time(NULL) - FABRIC_IR_CACHE_EXPIRY_SEC );
      RC::ConstHandle<IO::Dir> osDir = IO::Dir::Create( baseDir, buildOS );
      RC::ConstHandle<IO::Dir> archDir = IO::Dir::Create( osDir, runningArch );
      RC::ConstHandle<IO::Dir> compileOptionsDir = IO::Dir::Create( archDir, compileOptionsString );
      m_dir = IO::Dir::Create( compileOptionsDir, _(buildCacheGeneration) );
    }
    
    std::string IRCache::keyForAST( RC::ConstHandle<AST::GlobalList> const &ast ) const
    {
      Util::SimpleString astJSONString = ast->toJSON( false );
      return Util::md5HexDigest( astJSONString.getData(), astJSONString.getLength() );
    }
    
    void IRCache::subDirAndEntryFromKey( std::string const &key, RC::ConstHandle<IO::Dir> &subDir, std::string &entry ) const
    {
      subDir = IO::Dir::Create( m_dir, key.substr( 0, 2 ) );
      entry = key.substr( 2, 30 );
    }
    
    std::string IRCache::get( std::string const &key ) const
    {
      RC::ConstHandle<IO::Dir> subDir;
      std::string entry;
      subDirAndEntryFromKey( key, subDir, entry );
      
      std::string result;
      try
      {
        result = subDir->getFileContents( entry );
      }
      catch ( Exception e )
      {
      }
      return result;
    }
    
    void IRCache::put( std::string const &key, std::string const &ir ) const
    {
      RC::ConstHandle<IO::Dir> subDir;
      std::string entry;
      subDirAndEntryFromKey( key, subDir, entry );
      
      try
      {
        subDir->putFileContents( entry, ir );
      }
      catch ( Exception e )
      {
        // [pzion 20110323] This is a cache; it's OK to fail to cache an entry.
      }
    }
  };
};
