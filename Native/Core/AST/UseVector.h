/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_USE_VECTOR_H
#define _FABRIC_AST_USE_VECTOR_H

#include <Fabric/Base/RC/Vector.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <string>
#include <set>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace AST
  {
    class Use;
    class UseNameToLocationMap;
    
    class UseVector : public RC::Vector< RC::ConstHandle<Use> >
    {
    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<UseVector> Create( RC::ConstHandle<Use> const &first = 0, RC::ConstHandle<UseVector> const &remaining = 0 );

      void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
      void collectUses( UseNameToLocationMap &uses ) const;

    protected:
    
      UseVector();
    };
  };
};

#endif //_FABRIC_AST_USE_VECTOR_H
