/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ProducerWrapper.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace MR
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ProducerWrapper, GC::Object );

    ProducerWrapper::ProducerWrapper( FABRIC_GC_OBJECT_CLASS_PARAM )
      : GC::Object( FABRIC_GC_OBJECT_CLASS_ARG )
    {
    }

    void ProducerWrapper::toJSON( JSON::Encoder &jsonEncoder ) const
    {
      JSON::ObjectEncoder jsonObjectEncoder = jsonEncoder.makeObject();
      
      {
        JSON::Encoder typeNameEncoder = jsonObjectEncoder.makeMember( "kind" );
        typeNameEncoder.makeString( getKind() );
      }
     
      toJSONImpl( jsonObjectEncoder );
    }
      
    void ProducerWrapper::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "toJSON", 6 ) )
        jsonExecGetJSONDesc( arg, resultArrayEncoder );
      else GC::Object::jsonExec( cmd, arg, resultArrayEncoder );
    }
    
    void ProducerWrapper::jsonExecGetJSONDesc(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      JSON::Encoder jg = resultArrayEncoder.makeElement();
      toJSON( jg );
    }
  }
}
