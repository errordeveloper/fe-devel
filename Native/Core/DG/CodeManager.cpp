/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/CodeManager.h>
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Core/DG/IRCache.h>

namespace Fabric
{
  namespace DG
  {
    CodeManager::CodeManager(
      CG::CompileOptions const *compileOptions,
      bool optimizeSynchronously
      )
      : m_compileOptions( compileOptions )
      , m_optimizeSynchonrously( optimizeSynchronously )
    {
    }
    
    RC::ConstHandle<Code> CodeManager::compileSourceCode(
      RC::ConstHandle<Context> const &context,
      std::string const &filename,
      std::string const &sourceCode
      )
    {
      RC::ConstHandle<Code> result;
      
      SourceCodeToCodeMap::const_iterator it = m_sourceCodeToCodeMap.find( sourceCode );
      if ( it != m_sourceCodeToCodeMap.end() )
        result = it->second.makeStrong();
        
      if ( !result )
      {
        //FABRIC_DEBUG_LOG( "No compiled code in cache; compiling" );
        result = Code::Create( context, filename, sourceCode, m_optimizeSynchonrously, m_compileOptions );
        it = m_sourceCodeToCodeMap.insert( SourceCodeToCodeMap::value_type( sourceCode, result ) ).first;
        FABRIC_ASSERT( it != m_sourceCodeToCodeMap.end() );
      }
      //else FABRIC_DEBUG_LOG( "Retrieved compiled code from cache" );
        
      return result;
    }
  };
};
