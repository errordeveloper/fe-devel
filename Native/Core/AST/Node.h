/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_NODE_H
#define _FABRIC_AST_NODE_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Core/CG/Location.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace JSON
  {
    class ObjectDecoder;
  };
  
  namespace CG
  {
    class Diagnostics;
    class Error;
  }
  
  namespace AST
  {
    class Node : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      Node( CG::Location const &location );

      virtual char const *nodeTypeName() const = 0;
      virtual void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
      
      CG::Location const &getLocation() const
      {
        return m_location;
      }
      
      void addWarning( CG::Diagnostics &diagnostics, Util::SimpleString const &desc ) const;
      void addError( CG::Diagnostics &diagnostics, Util::SimpleString const &desc ) const;

    protected:
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const = 0;

      void addError( CG::Diagnostics &diagnostics, CG::Error const &error ) const;
      
    private:
    
      CG::Location m_location;
    };
  };
};

#define FABRIC_AST_NODE_DECL(NodeName) \
  public: \
    \
    static char const *NodeTypeName(); \
    virtual char const *nodeTypeName() const; \
    
#define FABRIC_AST_NODE_IMPL(NodeName) \
    char const *NodeName::NodeTypeName() \
    { \
      static char const *result = #NodeName; \
      return result; \
    } \
    \
    char const *NodeName::nodeTypeName() const \
    { \
      return NodeTypeName(); \
    } \

#endif //_FABRIC_AST_NODE_H
