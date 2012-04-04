/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/BCCache.h>
#include <Fabric/Core/IO/Dir.h>
#include <Fabric/Core/Util/MD5.h>
#include <Fabric/Core/AST/GlobalList.h>
#include <Fabric/Core/CG/CompileOptions.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Core/Build.h>

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/system_error.h>
#include <map>

#define FABRIC_BC_CACHE_EXPIRY_SEC (30*24*60*60)

namespace Fabric
{
  namespace DG
  {
    std::map< std::string, RC::Handle<BCCache> > g_bcInstances;

    RC::Handle<BCCache> BCCache::Instance( CG::CompileOptions const *compileOptions )
    {
      std::string compileOptionsString = compileOptions->getString();
      RC::Handle<BCCache> &instance = g_bcInstances[compileOptionsString];
      if ( !instance )
        instance = new BCCache( compileOptionsString );

      return instance;
    }

    void BCCache::Terminate()
    {
      g_bcInstances.clear();
    }
      
    BCCache::BCCache( std::string const &compileOptionsString )
    {
      RC::ConstHandle<IO::Dir> rootDir = IO::Dir::Private();
      RC::ConstHandle<IO::Dir> baseDir = IO::Dir::Create( rootDir, "IRCache" );
      baseDir->recursiveDeleteFilesOlderThan( time(NULL) - FABRIC_BC_CACHE_EXPIRY_SEC );
      RC::ConstHandle<IO::Dir> osDir = IO::Dir::Create( baseDir, buildOS );
      RC::ConstHandle<IO::Dir> archDir = IO::Dir::Create( osDir, runningArch );
      RC::ConstHandle<IO::Dir> compileOptionsDir = IO::Dir::Create( archDir, compileOptionsString );
      m_dir = IO::Dir::Create( compileOptionsDir, buildCacheGeneration );
    }
    
    std::string BCCache::keyForAST( RC::ConstHandle<AST::GlobalList> const &ast ) const
    {
      Util::SimpleString astJSONString = ast->toJSON( false );
      return Util::md5HexDigest( astJSONString.getData(), astJSONString.getLength() );
    }
    
    void BCCache::subDirAndEntryFromKey( std::string const &key, RC::ConstHandle<IO::Dir> &subDir, std::string &entry ) const
    {
      subDir = IO::Dir::Create( m_dir, key.substr( 0, 2 ) );
      entry = key.substr( 2, 30 );
    }
    
    llvm::MemoryBuffer *BCCache::get( std::string const &key ) const
    {
      RC::ConstHandle<IO::Dir> subDir;
      std::string entry;
      subDirAndEntryFromKey( key, subDir, entry );

      llvm::OwningPtr<llvm::MemoryBuffer> buffer;
      llvm::error_code fileError =
        llvm::MemoryBuffer::getFileOrSTDIN( subDir->getFullFilePath( entry ), buffer );
      if ( !fileError )
        return buffer.take();
      return NULL;
    }
    
    void BCCache::put( std::string const &key, llvm::Module *module ) const
    {
      RC::ConstHandle<IO::Dir> subDir;
      std::string entry;
      subDirAndEntryFromKey( key, subDir, entry );

      std::string errors;
      llvm::raw_fd_ostream bcStream(
        subDir->getFullFilePath( entry ).c_str(),
        errors,
        llvm::raw_fd_ostream::F_Binary
      );

      llvm::WriteBitcodeToFile( module, bcStream );
      bcStream.flush();
    }
  };
};
