/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_BC_CACHE_H
#define _FABRIC_DG_BC_CACHE_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <llvm/Module.h>
#include <llvm/Support/MemoryBuffer.h>

namespace Fabric
{
  namespace CG
  {
    class CompileOptions;
  };
  
  namespace AST
  {
    class GlobalList;
  };
  
  namespace IO
  {
    class Dir;
  };
  
  namespace DG
  {
    class BCCache : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<BCCache> Instance( CG::CompileOptions const *compileOptions );
      static void Terminate();
      
      std::string keyForAST( RC::ConstHandle<AST::GlobalList> const &ast ) const;
      
      llvm::MemoryBuffer *get( std::string const &key ) const;
      void put( std::string const &key, llvm::Module *module ) const;
      
    protected:
    
      BCCache( std::string const &compileOptionsString );
      
      void subDirAndEntryFromKey( std::string const &key, RC::ConstHandle<IO::Dir> &subDir, std::string &entry ) const;
      
    private:
    
      RC::ConstHandle<IO::Dir> m_dir;
    };
  };
};

#endif //_FABRIC_DG_BC_CACHE_H
