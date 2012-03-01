/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_COMPILE_OPTIONS_H
#define _FABRIC_CG_COMPILE_OPTIONS_H

#include <string>

namespace Fabric
{
  namespace CG
  {
    class CompileOptions
    {
    public:
    
      CompileOptions()
      {
        setDefaults();
      }
      
      CompileOptions( CompileOptions const &that )
        : m_guarded( that.m_guarded )
      {
      }
      
      CompileOptions( std::string const &string )
      {
        setDefaults();
        setFromString( string );
      }
      
      std::string getString() const;
      void setFromString( std::string const &string );
      
      bool getGuarded() const
      {
        return m_guarded;
      }
      
      void setGuarded( bool guarded )
      {
        m_guarded = guarded;
      }
      
      void setDefaults()
      {
        setGuarded( true );
      }
    
    private:
    
      bool m_guarded;      
    };
  };
};

#endif //_FABRIC_CG_COMPILE_OPTIONS_H
