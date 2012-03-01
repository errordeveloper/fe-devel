/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_LOCATION_H
#define _FABRIC_CG_LOCATION_H

#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/RC/String.h>

namespace Fabric
{
  namespace CG
  {
    class Location
    {
    public:
    
      Location( RC::ConstHandle<RC::String> const &filename, size_t line, size_t column )
        : m_filename( filename )
        , m_line( line )
        , m_column( column )
      {
        FABRIC_ASSERT( m_filename );
      }
      
      Location( Location const &that )
        : m_filename( that.m_filename )
        , m_line( that.m_line )
        , m_column( that.m_column )
      {
      }
      
      Location &operator =( Location const &that )
      {
        m_filename = that.m_filename;
        m_line = that.m_line;
        m_column = that.m_column;
        return *this;
      }
      
      RC::ConstHandle<RC::String> getFilename() const
      {
        return m_filename;
      }
      
      size_t getLine() const
      {
        return m_line;
      }
      
      size_t getColumn() const
      {
        return m_column;
      }
      
      std::string desc() const
      {
        return m_filename->stdString() + ":" + _(m_line) + ":" + _(m_column);
      }
      
      void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const
      {
        JSON::ArrayEncoder jsonArrayEncoder = encoder.makeArray();
        jsonArrayEncoder.makeElement().makeString( m_filename );
        jsonArrayEncoder.makeElement().makeInteger( m_line );
        jsonArrayEncoder.makeElement().makeInteger( m_column );
      }
      
    private:
    
      RC::ConstHandle<RC::String> m_filename;
      size_t m_line;
      size_t m_column;
    };
  };
};

#endif //_FABRIC_CG_LOCATION_H
