/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_CODE_MANAGER_H
#define _FABRIC_DG_CODE_MANAGER_H

#include <Fabric/Core/DG/Code.h>
#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Base/RC/WeakConstHandle.h>
#include <Fabric/Core/Util/UnorderedMap.h>

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
  
  namespace DG
  {
    class Context;
    
    class CodeManager : public RC::Object
    {
      typedef Util::UnorderedMap< std::string, RC::WeakConstHandle<Code> > SourceCodeToCodeMap;
      
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<CodeManager> Create(
        CG::CompileOptions const *compileOptions,
        bool optimizeSynchronously
        )
      {
        return new CodeManager( compileOptions, optimizeSynchronously );
      }
      
      RC::ConstHandle<Code> compileSourceCode( RC::ConstHandle<Context> const &context, std::string const &filename, std::string const &sourceCode );
      
    protected:
    
      CodeManager(
        CG::CompileOptions const *compileOptions,
        bool optimizeSynchronously
        );
      
    private:
    
      SourceCodeToCodeMap m_sourceCodeToCodeMap;
      CG::CompileOptions const *m_compileOptions;
      bool m_optimizeSynchonrously;
    };
  };
};

#endif //_FABRIC_DG_CODE_MANAGER_H
