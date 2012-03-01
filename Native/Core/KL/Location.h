/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KL_LOCATION_H
#define _FABRIC_KL_LOCATION_H

#include <Fabric/Core/CG/Location.h>

#include <stdlib.h>

namespace Fabric
{
  namespace KL
  {
    class Location
    {
    public:
    
      Location( RC::ConstHandle<RC::String> const &filename, size_t index = 0, size_t line = 1, size_t column = 1 )
        : m_filename( filename )
        , m_index( index )
        , m_line( line )
        , m_column( column )
      {
        FABRIC_ASSERT( m_filename );
      }
      
      Location( Location const &that )
        : m_filename( that.m_filename )
        , m_index( that.m_index )
        , m_line( that.m_line )
        , m_column( that.m_column )
      {
        FABRIC_ASSERT( m_filename );
      }
      
      Location &operator =( Location const &that )
      {
        m_filename = that.m_filename;
        m_index = that.m_index;
        m_line = that.m_line;
        m_column = that.m_column;
        return *this;
      }
      
      RC::ConstHandle<RC::String> getFilename() const
      {
        return m_filename;
      }
      
      size_t getIndex() const
      {
        return m_index;
      }
      
      void setIndex( size_t index )
      {
        m_index = index;
      }
      
      size_t getLine() const
      {
        return m_line;
      }
      
      void setLine( size_t line )
      {
        m_line = line;
      }
      
      size_t getColumn() const
      {
        return m_column;
      }
      
      void setColumn( size_t column )
      {
        m_column = column;
      }
      
      operator CG::Location() const
      {
        return CG::Location( m_filename, m_line, m_column );
      }
      
    private:
    
      RC::ConstHandle<RC::String> m_filename;
      size_t m_index;
      size_t m_line;
      size_t m_column;
    };
  };
};

#endif //_FABRIC_KL_LOCATION_H
