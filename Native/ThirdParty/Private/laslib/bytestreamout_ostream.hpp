/*
===============================================================================

  FILE:  bytestreamout_ostream.hpp
  
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
  
    10 January 2011 -- licensing change for LGPL release and liblas integration
    12 December 2010 -- created from ByteStreamOutFile after Howard got pushy (-;
  
===============================================================================
*/
#ifndef BYTE_STREAM_OUT_OSTREAM_H
#define BYTE_STREAM_OUT_OSTREAM_H

#include "bytestreamout.hpp"

#ifdef LZ_WIN32_VC6
#include <fstream.h>
#else
#include <istream>
#include <fstream>
using namespace std;
#endif

class ByteStreamOutOstream : public ByteStreamOut
{
public:
  ByteStreamOutOstream(ostream& stream);
/* write a single byte                                       */
  bool putByte(unsigned char byte);
/* write an array of bytes                                   */
  bool putBytes(const unsigned char* bytes, unsigned int num_bytes);
/* is the stream seekable (e.g. standard out is not)         */
  bool isSeekable() const;
/* get current position of stream                            */
  long position() const;
/* seek to this position in the stream                       */
  bool seek(const long position);
/* seek to the end of the file                               */
  bool seekEnd();
/* returns how many bytes were written since last reset      */
  unsigned int byteCount() const;
/* reset byte counter                                        */
  void resetCount();
/* destructor                                                */
  ~ByteStreamOutOstream(){};
protected:
  ostream& stream;
private:
#ifdef LZ_WIN32_VC6
  long start;
#else
  ios::off_type start;
#endif
};

class ByteStreamOutOstreamLE : public ByteStreamOutOstream
{
public:
  ByteStreamOutOstreamLE(ostream& stream);
/* write 16 bit low-endian field                             */
  bool put16bitsLE(const unsigned char* bytes);
/* write 32 bit low-endian field                             */
  bool put32bitsLE(const unsigned char* bytes);
/* write 64 bit low-endian field                             */
  bool put64bitsLE(const unsigned char* bytes);
/* write 16 bit big-endian field                             */
  bool put16bitsBE(const unsigned char* bytes);
/* write 32 bit big-endian field                             */
  bool put32bitsBE(const unsigned char* bytes);
/* write 64 bit big-endian field                             */
  bool put64bitsBE(const unsigned char* bytes);
private:
  unsigned char swapped[8];
};

class ByteStreamOutOstreamBE : public ByteStreamOutOstream
{
public:
  ByteStreamOutOstreamBE(ostream& stream);
/* write 16 bit low-endian field                             */
  bool put16bitsLE(const unsigned char* bytes);
/* write 32 bit low-endian field                             */
  bool put32bitsLE(const unsigned char* bytes);
/* write 64 bit low-endian field                             */
  bool put64bitsLE(const unsigned char* bytes);
/* write 16 bit big-endian field                             */
  bool put16bitsBE(const unsigned char* bytes);
/* write 32 bit big-endian field                             */
  bool put32bitsBE(const unsigned char* bytes);
/* write 64 bit big-endian field                             */
  bool put64bitsBE(const unsigned char* bytes);
private:
  unsigned char swapped[8];
};

inline ByteStreamOutOstream::ByteStreamOutOstream(ostream& stream_param) :
    stream(stream_param)
{
  start = stream.tellp();
}

inline bool ByteStreamOutOstream::putByte(unsigned char byte)
{
  stream.put(byte);
  return !!(stream.good());
}

inline bool ByteStreamOutOstream::putBytes(const unsigned char* bytes, unsigned int num_bytes)
{
  stream.write((const char*)bytes, num_bytes);
  return !!(stream.good());
}

inline bool ByteStreamOutOstream::isSeekable() const
{
  return (!!(static_cast<ofstream&>(stream)));
}

inline long ByteStreamOutOstream::position() const
{
  return (long)stream.tellp();
}

inline bool ByteStreamOutOstream::seek(long position)
{
  stream.seekp(position);
  return !!(stream.good());
}

inline bool ByteStreamOutOstream::seekEnd()
{
  stream.seekp(0, ios::end);
  return !!(stream.good());
}

inline unsigned int ByteStreamOutOstream::byteCount() const
{
  return (unsigned int)(stream.tellp() - start);
}

inline void ByteStreamOutOstream::resetCount()
{
  start = stream.tellp();
}

inline ByteStreamOutOstreamLE::ByteStreamOutOstreamLE(ostream& stream) : ByteStreamOutOstream(stream)
{
}

inline bool ByteStreamOutOstreamLE::put16bitsLE(const unsigned char* bytes)
{
  return putBytes(bytes, 2);
}

inline bool ByteStreamOutOstreamLE::put32bitsLE(const unsigned char* bytes)
{
  return putBytes(bytes, 4);
}

inline bool ByteStreamOutOstreamLE::put64bitsLE(const unsigned char* bytes)
{
  return putBytes(bytes, 8);
}

inline bool ByteStreamOutOstreamLE::put16bitsBE(const unsigned char* bytes)
{
  swapped[0] = bytes[1];
  swapped[1] = bytes[0];
  return putBytes(swapped, 2);
}

inline bool ByteStreamOutOstreamLE::put32bitsBE(const unsigned char* bytes)
{
  swapped[0] = bytes[3];
  swapped[1] = bytes[2];
  swapped[2] = bytes[1];
  swapped[3] = bytes[0];
  return putBytes(swapped, 4);
}

inline bool ByteStreamOutOstreamLE::put64bitsBE(const unsigned char* bytes)
{
  swapped[0] = bytes[7];
  swapped[1] = bytes[6];
  swapped[2] = bytes[5];
  swapped[3] = bytes[4];
  swapped[4] = bytes[3];
  swapped[5] = bytes[2];
  swapped[6] = bytes[1];
  swapped[7] = bytes[0];
  return putBytes(swapped, 8);
}

inline ByteStreamOutOstreamBE::ByteStreamOutOstreamBE(ostream& stream) : ByteStreamOutOstream(stream)
{
}

inline bool ByteStreamOutOstreamBE::put16bitsLE(const unsigned char* bytes)
{
  swapped[0] = bytes[1];
  swapped[1] = bytes[0];
  return putBytes(swapped, 2);
}

inline bool ByteStreamOutOstreamBE::put32bitsLE(const unsigned char* bytes)
{
  swapped[0] = bytes[3];
  swapped[1] = bytes[2];
  swapped[2] = bytes[1];
  swapped[3] = bytes[0];
  return putBytes(swapped, 4);
}

inline bool ByteStreamOutOstreamBE::put64bitsLE(const unsigned char* bytes)
{
  swapped[0] = bytes[7];
  swapped[1] = bytes[6];
  swapped[2] = bytes[5];
  swapped[3] = bytes[4];
  swapped[4] = bytes[3];
  swapped[5] = bytes[2];
  swapped[6] = bytes[1];
  swapped[7] = bytes[0];
  return putBytes(swapped, 8);
}

inline bool ByteStreamOutOstreamBE::put16bitsBE(const unsigned char* bytes)
{
  return putBytes(bytes, 2);
}

inline bool ByteStreamOutOstreamBE::put32bitsBE(const unsigned char* bytes)
{
  return putBytes(bytes, 4);
}

inline bool ByteStreamOutOstreamBE::put64bitsBE(const unsigned char* bytes)
{
  return putBytes(bytes, 8);
}

#endif
