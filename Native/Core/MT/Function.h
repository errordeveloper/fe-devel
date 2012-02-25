/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MT_FUNCTION_H
#define _FABRIC_MT_FUNCTION_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/Util/Mutex.h>

#include <set>

namespace Fabric
{
  namespace MT
  {
    class ParallelCall;
    
    class Function : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      typedef void (*FunctionPtr)( ... );
    
      FunctionPtr getFunctionPtr( RC::ConstHandle<RC::Object> &handleToObjectOwningFunctionPtr ) const;
      
    protected:
    
      Function();
    
      void onFunctionPtrChange( FunctionPtr functionPtr, RC::ConstHandle<RC::Object> const &objectOwningFunctionPtr );
      
    private:
    
      mutable Util::Mutex m_mutex;
      FunctionPtr m_functionPtr;
      RC::ConstHandle<RC::Object> m_objectOwningFunctionPtr;
    };
  };
};

#endif //_FABRIC_MT_FUNCTION_H
