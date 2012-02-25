/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_POLYMORPHISM_PARAMS_H
#define _FABRIC_CG_POLYMORPHISM_PARAMS_H

namespace Fabric
{
  namespace CG
  {
    struct PolymorphismParameters
    {
      enum ParamType
      {
        PT_COST,
        PT_MIN_EXACT_TYPE_MATCH
      };
      
      size_t cost;
      size_t minExactTypeMatch;
      
      PolymorphismParameters()
        : cost( 0 )
        , minExactTypeMatch( 0 )
      {
      }
      
      PolymorphismParameters( ParamType paramType, size_t param )
      {
        switch ( paramType )
        {
          case PT_COST:
            cost = param;
            minExactTypeMatch = 0;
            break;
          case PT_MIN_EXACT_TYPE_MATCH:
            cost = 0;
            minExactTypeMatch = param;
            break;
        }
      }
      
      PolymorphismParameters( PolymorphismParameters const &that )
        : cost( that.cost )
        , minExactTypeMatch( that.minExactTypeMatch )
      {
      }
      
      PolymorphismParameters &operator =( PolymorphismParameters const &that )
      {
        cost = that.cost;
        minExactTypeMatch = that.minExactTypeMatch;
        return *this;
      }
    };
  }
}

#endif //_FABRIC_CG_POLYMORPHISM_PARAMS_H
