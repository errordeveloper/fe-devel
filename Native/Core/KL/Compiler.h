/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KL_COMPILER_H
#define _FABRIC_KL_COMPILER_H

#include <Fabric/Core/RT/Manager.h>

namespace Fabric
{
  namespace KL
  {
    class Compiler : public RT::Manager::KLCompiler
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<Compiler> Create();
      
      virtual RC::ConstHandle<RC::Object> compile( std::string const &klFilename, std::string const &klSource ) const;

    protected:
    
      Compiler();
      
    private:
    
    };
  }
}

#endif //_FABRIC_KL_COMPILER_H
