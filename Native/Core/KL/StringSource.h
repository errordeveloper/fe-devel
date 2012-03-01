/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KL_STRING_SOURCE_H
#define _FABRIC_KL_STRING_SOURCE_H

#include <Fabric/Core/KL/Source.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Base/RC/String.h>

#include <string>

namespace Fabric
{
  namespace KL
  {
    class StringSource : public Source
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<StringSource> Create( std::string const &filename, std::string const &source )
      {
        return new StringSource( filename, source );
      }
      
    protected:
    
      StringSource( std::string const &filename, std::string const &source )
        : m_filename( RC::String::Create( filename ) )
        , m_source( source )
      {
      }
      
      virtual RC::ConstHandle<RC::String> filename() const
      {
        return m_filename;
      }
      
      virtual char const *data() const
      {
        return m_source.data();
      }
      
      virtual size_t length() const
      {
        return m_source.length();
      }
      
    private:
    
      RC::ConstHandle<RC::String> m_filename;
      std::string m_source;
    };
  };
};

#endif //_FABRIC_KL_STRING_SOURCE_H
