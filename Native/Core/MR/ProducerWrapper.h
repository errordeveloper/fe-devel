/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_MR_PRODUCER_WRAPPER_H
#define _FABRIC_MR_PRODUCER_WRAPPER_H

#include <Fabric/Core/GC/Object.h>

namespace Fabric
{
  namespace Util
  {
    class JSONGenerator;
    class JSONObjectGenerator;
  };
  
  namespace MR
  {
    class ProducerWrapper : public GC::Object
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
      // Virtual functions: GC::Object
    
    public:
    
      virtual void jsonExec(
        std::string const &cmd,
        RC::ConstHandle<JSON::Value> const &arg,
        Util::JSONArrayGenerator &resultJAG
        );
      
    protected:
    
      ProducerWrapper( FABRIC_GC_OBJECT_CLASS_PARAM ); 
    
      virtual char const *getKind() const = 0;
      virtual void toJSONImpl( Util::JSONObjectGenerator &jog ) const = 0;
      
      // Non-virtual functions
    
    public:
    
      void toJSON( Util::JSONGenerator &jg ) const;
      
    private:

      void jsonExecGetJSONDesc(
        RC::ConstHandle<JSON::Value> const &arg,
        Util::JSONArrayGenerator &resultJAG
        );
    };
  }
}

#endif //_FABRIC_MR_PRODUCER_WRAPPER_H