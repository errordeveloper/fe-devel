/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_PRODUCER_WRAPPER_H
#define _FABRIC_MR_PRODUCER_WRAPPER_H

#include <Fabric/Core/GC/Object.h>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
    class ObjectEncoder;
  };
  
  namespace MR
  {
    class ProducerWrapper : public GC::Object
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
      // Virtual functions: GC::Object
    
    public:
      REPORT_RC_LEAKS
    
      virtual void jsonExec(
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
    protected:
    
      ProducerWrapper( FABRIC_GC_OBJECT_CLASS_PARAM ); 
    
      virtual char const *getKind() const = 0;
      virtual void toJSONImpl( JSON::ObjectEncoder &objectEncoder ) const = 0;
      
      // Non-virtual functions
    
    public:
    
      void toJSON( JSON::Encoder &jg ) const;
      
    private:

      void jsonExecGetJSONDesc(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
    };
  }
}

#endif //_FABRIC_MR_PRODUCER_WRAPPER_H
