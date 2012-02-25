/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_CONTEXT_H
#define _FABRIC_CG_CONTEXT_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>

#include <llvm/LLVMContext.h>
#include <map>
#include <string>

namespace llvm
{
  class Type;
};

namespace Fabric
{
  namespace CG
  {
    class Context : public RC::Object
    {
      typedef std::map< std::string, llvm::Type const * > LLVMTypeMap;
    
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<Context> Create();
      
      llvm::LLVMContext &getLLVMContext()
      {
        return m_llvmContext;
      }
      
      llvm::Type const *getLLVMRawType( std::string const &codeName ) const
      {
        LLVMTypeMap::const_iterator it = m_llvmRawTypes.find( codeName );
        if ( it != m_llvmRawTypes.end() )
          return it->second;
        else return 0;
      }
      
      void setLLVMRawType( std::string const &codeName, llvm::Type const *llvmType )
      {
        m_llvmRawTypes[codeName] = llvmType;
      }
      
    protected:
    
      Context();
      
    private:
    
      llvm::LLVMContext m_llvmContext;
      LLVMTypeMap m_llvmRawTypes;
    };
  };
};

#endif //_FABRIC_CG_CONTEXT_H
