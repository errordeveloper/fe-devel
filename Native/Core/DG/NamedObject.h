/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_NAMED_OBJECT_H
#define _FABRIC_DG_NAMED_OBJECT_H

#include <Fabric/Core/DG/CompiledObject.h>
#include <Fabric/Core/DG/Context.h>

#include <string>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace DG
  {
    class NamedObject : public CompiledObject
    {
    public:
      REPORT_RC_LEAKS

      std::string const &getName() const;
      
      virtual bool isNamedObject() const { return true; }
      virtual bool isContainer() const { return false; }
      virtual bool isOperator() const { return false; }
            
      virtual void jsonRoute(
        std::vector<JSON::Entity> const &dst,
        size_t dstOffset,
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
        
      virtual void jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonDesc( JSON::Encoder &resultEncoder ) const;
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      virtual void jsonDescType( JSON::Encoder &resultEncoder ) const = 0;
      virtual void jsonGetMemoryUsage( JSON::Encoder &jg ) const;
      
    protected:
    
      NamedObject( std::string const &friendlyName, RC::Handle<Context> const &context );
      
      void jsonNotify( char const *cmdData, size_t cmdLength, Util::SimpleString const *argJSON = 0 ) const;
      void jsonNotifyDelta( Util::SimpleString const &deltaJSON ) const;
      void jsonNotifyMemberDelta( char const *memberData, size_t memberLength, Util::SimpleString const &memberDeltaJSON ) const;
      virtual void jsonNotifyErrorDelta() const;
      
    private:
    
      Context *m_context;
      std::vector<std::string> m_notificationSrc;
    };
  };
};

#endif //_FABRIC_DG_NAMED_OBJECT_H
