/*
===============================================================================

  FILE:  bytestreamin.hpp
  
  CONTENTS:
      
  PROGRAMMERS:
  
    martin.isenburg@gmail.com
  
  COPYRIGHT:

    (c) 2010-2011, Martin Isenburg, LASSO - tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
     1 October 2011 -- added 64 bit file support in MSVC 6.0 at McCafe at Hbf Linz
    10 January 2011 -- licensing change for LGPL release and liblas integration
    12 December 2010 -- created from ByteStreamOutFile after Howard got pushy (-;
  
===============================================================================
*/
#ifndef BYTE_STREAM_IN_HPP
#define BYTE_STREAM_IN_HPP

#include "mydefs.hpp"

class ByteStreamIn
{
public:
/* read a single byte                                        */
  virtual U32 getByte() = 0;
/* read an array of bytes                                    */
  virtual BOOL getBytes(U8* bytes, const U32 num_bytes) = 0;
/* read 16 bit low-endian field                              */
  virtual BOOL get16bitsLE(U8* bytes) = 0;
/* read 32 bit low-endian field                              */
  virtual BOOL get32bitsLE(U8* bytes) = 0;
/* read 64 bit low-endian field                              */
  virtual BOOL get64bitsLE(U8* bytes) = 0;
/* read 16 bit big-endian field                              */
  virtual BOOL get16bitsBE(U8* bytes) = 0;
/* read 32 bit big-endian field                              */
  virtual BOOL get32bitsBE(U8* bytes) = 0;
/* read 64 bit big-endian field                              */
  virtual BOOL get64bitsBE(U8* bytes) = 0;
/* is the stream seekable (e.g. stdin is not)                */
  virtual BOOL isSeekable() const = 0;
/* get current position of stream                            */
  virtual I64 tell() const = 0;
/* seek to this position in the stream                       */
  virtual BOOL seek(const I64 position) = 0;
/* seek to the end of the file                               */
  virtual BOOL seekEnd(const I64 distance=0) = 0;
/* destructor                                                */
  virtual ~ByteStreamIn() {};
};

#endif
