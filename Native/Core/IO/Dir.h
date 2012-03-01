/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_DIR_H
#define _FABRIC_IO_DIR_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <vector>
#include <string>

namespace Fabric
{
  namespace IO
  {
    class Dir : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<Dir> Root();
      static RC::ConstHandle<Dir> User();
      static RC::ConstHandle<Dir> Private();
          
      static RC::ConstHandle<Dir> Create( RC::ConstHandle<Dir> const &parentDir, std::string path, bool createIfMissing = true )
      {
        return new Dir( parentDir, path, createIfMissing );
      }

      static RC::ConstHandle<Dir> Create( std::string path, bool createIfMissing = true )
      {
        return Create( RC::ConstHandle<Dir>(), path, createIfMissing );
      }

      RC::ConstHandle<Dir> getParentDir() const
      {
        return m_parentDir;
      }
      
      std::string const &getEntry() const
      {
        return m_entry;
      }

      bool exists() const;
      
      std::string getFullPath() const;
      
      std::vector< std::string > getFiles() const;
      std::vector< RC::ConstHandle<Dir> > getSubDirs( bool followLinks = true ) const;
      
      std::string getFileContents( std::string const &entry ) const;
      void putFileContents( std::string const &entry, std::string const &contents ) const;
      
      void recursiveDeleteFilesOlderThan( time_t time ) const;
    
    protected:
    
      Dir( RC::ConstHandle<Dir> const &parentDir, std::string const &entry, bool createIfMissing );
      
    private:
    
      RC::ConstHandle<Dir> m_parentDir;
      std::string m_entry;
    };
  };
};

#endif //_FABRIC_IO_DIR_H
