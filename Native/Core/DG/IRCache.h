/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_IR_CACHE_H
#define _FABRIC_DG_IR_CACHE_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <string>

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
    class IRCache : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<IRCache> Instance( CG::CompileOptions const *compileOptions );
      static void Terminate();
      
      std::string keyForAST( RC::ConstHandle<AST::GlobalList> const &ast ) const;
      
      std::string get( std::string const &key ) const;
      void put( std::string const &key, std::string const &ir ) const;
      
    protected:
    
      IRCache( std::string const &compileOptionsString );
      
      void subDirAndEntryFromKey( std::string const &key, RC::ConstHandle<IO::Dir> &subDir, std::string &entry ) const;
      
    private:
    
      RC::ConstHandle<IO::Dir> m_dir;
    };
  };
};

#endif //_FABRIC_DG_IR_CACHE_H
