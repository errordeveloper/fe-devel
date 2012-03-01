/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_PLUG_DESC_H
#define _FABRIC_PLUG_DESC_H
 
#include <Fabric/Base/Config.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <map>
#include <vector>
#include <string>

namespace Fabric
{
  namespace JSON
  {
    struct Entity;
  };
  
  namespace Plug
  {
    class HostSpec : public std::vector< std::string >
    {
    public:
    
      HostSpec()
      {
      }
      
      HostSpec( std::string pattern )
      {
        push_back( pattern );
      }
      
      HostSpec( HostSpec const &that )
        : std::vector< std::string >( that )
      {
      }
      
      HostSpec &operator =( HostSpec const &that )
      {
        *(std::vector< std::string > *)this = (std::vector< std::string > const &)that;
        return *this;
      }
      
      bool match( std::string const &hostTriple ) const
      {
        bool result = false;
        for ( const_iterator it=begin(); it!=end(); ++it )
        {
          if ( Util::matchHostTriple( hostTriple, *it ) )
          {
            result = true;
            break;
          }
        }
        return result;
      }
    };
    
    class HostStrings : public std::map< std::string, HostSpec >
    {
    public:
    
      HostStrings()
        : std::map< std::string, HostSpec >()
      {
      }
      
      void appendMatching( std::string const &hostTriple, std::vector<std::string> &strings ) const
      {
        for ( const_iterator it=begin(); it!=end(); ++it )
          if ( it->second.match( hostTriple ) )
            strings.push_back( it->first );
      }
      
      std::string concatMatching( std::string const &hostTriple ) const
      {
        std::string result;
        for ( const_iterator it=begin(); it!=end(); ++it )
          if ( it->second.match( hostTriple ) )
            result += it->first;
        return result;
      }
    
    };
    
    class HostStringsVector : public std::vector<HostStrings>
    {
    public:
    
      HostStringsVector()
        : std::vector<HostStrings>()
      {
      }
      
      void appendMatching( std::string const &hostTriple, std::vector<std::string> &strings ) const
      {
        for ( const_iterator it=begin(); it!=end(); ++it )
          it->appendMatching( hostTriple, strings );
      }
      
      std::string concatMatching( std::string const &hostTriple ) const
      {
        std::string result;
        for ( const_iterator it=begin(); it!=end(); ++it )
          result += it->concatMatching( hostTriple );
        return result;
      }
    };
    
    struct Interface
    {
      std::vector< std::string > methods;
    };
  
    struct Desc
    {
      HostStringsVector libs;
      HostStringsVector code;
      HostStringsVector jsConstants;
      Interface interface;
    };
    
    Desc parseDesc( JSON::Entity const &object );
  };
};

#endif //_FABRIC_PLUG_DESC_H
