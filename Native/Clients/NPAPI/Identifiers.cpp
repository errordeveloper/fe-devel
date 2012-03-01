/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Identifiers.h"
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Core/Util/UnorderedMap.h>

namespace Fabric
{
  namespace NPAPI
  {
    typedef Util::UnorderedMap< NPIdentifier, std::string * > IdentifierToStringMap;
      
    std::string const *identifierToString( NPIdentifier identifier )
    {
      static IdentifierToStringMap identifierToStringMap; 
      IdentifierToStringMap::const_iterator it = identifierToStringMap.find( identifier );
      if ( it == identifierToStringMap.end() )
      {
        NPUTF8 *utf8String = NPN_UTF8FromIdentifier( identifier );
        
        std::string *string;
        if ( utf8String )
        {
          string = new std::string( utf8String );
          NPN_MemFree( utf8String );
        }
        else string = 0;

        it = identifierToStringMap.insert( IdentifierToStringMap::value_type( identifier, string ) ).first;
      }
      return it->second;
    }
    
    typedef Util::UnorderedMap< std::string, NPIdentifier > StringToIdentifierMap;

    NPIdentifier stringToIdentifier( std::string const &string )
    {
      static StringToIdentifierMap stringToIdentifierMap; 
      StringToIdentifierMap::const_iterator it = stringToIdentifierMap.find( string );
      if ( it == stringToIdentifierMap.end() )
      {
        NPIdentifier identifier = NPN_GetStringIdentifier( string.c_str() );
        FABRIC_ASSERT( identifier );
        it = stringToIdentifierMap.insert( StringToIdentifierMap::value_type( string, identifier ) ).first;
      }
      return it->second;
    }

    typedef Util::UnorderedMap< size_t, NPIdentifier > IntegerToIdentifierMap;

    NPIdentifier integerToIdentifier( size_t integer )
    {
      static IntegerToIdentifierMap integerToIdentifierMap; 
      IntegerToIdentifierMap::const_iterator it = integerToIdentifierMap.find( integer );
      if ( it == integerToIdentifierMap.end() )
      {
        NPIdentifier identifier = NPN_GetIntIdentifier( integer );
        FABRIC_ASSERT( identifier );
        it = integerToIdentifierMap.insert( IntegerToIdentifierMap::value_type( integer, identifier ) ).first;
      }
      return it->second;
    }
  };
};
