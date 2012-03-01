/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MT/ParallelCall.h>

namespace Fabric
{
  namespace MT
  {
    Util::TLSVar<void *> ParallelCall::s_userdataTLS;
    
    ParallelCall::ParallelCall(
      RC::ConstHandle<Function> const &function,
      size_t paramCount,
      std::string const &debugDesc
      )
      : m_function( function )
      , m_paramCount( paramCount )
      , m_totalParallelCalls( 1 )
      , m_debugDesc( debugDesc )
    {
    }
    
    ParallelCall::~ParallelCall()
    {
    }
  };
};
