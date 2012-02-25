/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_USE_H
#define _FABRIC_AST_USE_H

#include <Fabric/Core/AST/Node.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace AST
  {
    class UseNameToLocationMap;
    
    class Use : public Node
    {
      FABRIC_AST_NODE_DECL( Use );
      
    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<Use> Create(
        CG::Location const &location,
        std::string const &name
        );
      
      void collectUses( UseNameToLocationMap &uses ) const;
     
    protected:
    
      Use(
        CG::Location const &location,
        std::string const &name
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      std::string m_name;
    };
  };
};

#endif //_FABRIC_AST_USE_H
