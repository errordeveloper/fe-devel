/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_CLIENT_H
#define _FABRIC_DG_CLIENT_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <vector>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  }
  
  namespace JSON
  {
    class ArrayEncoder;
    class Encoder;
    struct Entity;
  };
  
  namespace DG
  {
    class Context;
    
    class Client : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      RC::Handle<Context> getContext() const;
      
      void jsonExec(
        char const *jsonEncodedCommandsData,
        size_t jsonEncodedCommandsLength,
        JSON::Encoder &resultEncoder
        ) const;

      virtual void notify( Util::SimpleString const &jsonEncodedNotifications ) const = 0;
      
      void jsonRoute(
        std::vector<JSON::Entity> const &dst,
        size_t dstOffset,
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      virtual void jsonExec(
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      void jsonDesc( JSON::Encoder &resultEncoder ) const;

    protected:
    
      Client( RC::Handle<Context> const &context );
      virtual ~Client();
      
      void notifyInitialState() const;
      
    private:
    
      RC::Handle<Context> m_context;
    };
  };
};

#endif //_FABRIC_DG_CONTEXT_H
