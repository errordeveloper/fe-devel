/*
===============================================================================

  FILE:  laswritersingle.hpp
  
  CONTENTS:
  
    Writes LIDAR points to the LAS format (Version 1.x , April 29, 2008).

  PROGRAMMERS:

    martin.isenburg@gmail.com

  COPYRIGHT:

    (c) 2007-2011, Martin Isenburg, LASSO - tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    8 May 2011 -- added an option for variable chunking via chunk()
    23 April 2011 -- added additional open parameters to support chunking
    21 January 2011 -- adapted from laswriter to create abstract reader class
    3 December 2010 -- updated to (somewhat) support LAS format 1.3
    7 September 2008 -- updated to support LAS format 1.2 
    21 February 2007 -- created after eating Sarah's veggies with peanutsauce
  
===============================================================================
*/
#ifndef LAS_WRITER_SINGLE_HPP
#define LAS_WRITER_SINGLE_HPP

#include "laswriter.hpp"

#include <stdio.h>

#ifdef LZ_WIN32_VC6
#include <fstream.h>
#else
#include <istream>
#include <fstream>
using namespace std;
#endif

class ByteStreamOut;
class LASwritePoint;

class LASwriterSingle : public LASwriter
{
public:

  BOOL refile(FILE* file);

  BOOL open(const LASheader* header, U32 compressor=LASZIP_COMPRESSOR_NONE, I32 requested_version=0, I32 chunk_size=50000);
  BOOL open(const char* file_name, const LASheader* header, U32 compressor=LASZIP_COMPRESSOR_NONE, I32 requested_version=0, I32 chunk_size=50000);
  BOOL open(FILE* file, const LASheader* header, U32 compressor=LASZIP_COMPRESSOR_NONE, I32 requested_version=0, I32 chunk_size=50000);
  BOOL open(ostream& ostream, const LASheader* header, U32 compressor=LASZIP_COMPRESSOR_NONE, I32 requested_version=0, I32 chunk_size=50000);

  BOOL write_point(const LASpoint* point);
  BOOL chunk();

  BOOL update_header(const LASheader* header, BOOL use_inventory=TRUE);
  U32 close(BOOL update_npoints=true);

  LASwriterSingle();
  ~LASwriterSingle();

private:
  BOOL open(ByteStreamOut* stream, const LASheader* header, U32 compressor, I32 requested_version, I32 chunk_size);
  ByteStreamOut* stream;
  LASwritePoint* writer;
  FILE* file;
  long header_start_position;
};

#endif
