/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

#include <stdint.h>

#ifdef _WIN32
  #ifndef INT64_C
    #define INT64_C(c) (c ## LL)
    #define UINT64_C(c) (c ## ULL)
  #endif
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

static AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height)
{
  AVFrame *picture;
  uint8_t *picture_buf;
  int size;
  
  picture = avcodec_alloc_frame();
  if (!picture)
  {
    Fabric::EDK::throwException("Video Extension: Could not allocate frame!");
    return NULL;
  }
  size = avpicture_get_size(pix_fmt, width, height);
  picture_buf = (uint8_t*)av_malloc(size);
  if (!picture_buf) {
    av_free(picture);
    Fabric::EDK::throwException("Video Extension: Could not allocate frame buffer!");
    return NULL;
  }
  avpicture_fill((AVPicture *)picture, picture_buf,
  pix_fmt, width, height);
  return picture;
}

class videoStream;

const float cFrameSkipTolerance = 0.005f;

struct videoHandle
{
  videoStream * pointer;
  KL::Size width;
  KL::Size height;
  KL::Scalar duration;
  KL::Scalar fps;
  KL::Scalar time;
};

class videoStream
{
private:
  static bool sRegistered;
  bool mReadOnly;
  AVFormatContext * mFormatCtx;
  AVCodecContext * mCodecCtx;
  AVStream * mVideoStream;
  int mVideoStreamID;
  AVCodec * mCodec;
  AVFrame * mFrameYUV;
  AVFrame * mFrameRGB;
  uint8_t * mBuffer;
  size_t mBufferSize;
  SwsContext * mConvertCtx;
  videoHandle * mHandle;

public:
    
  videoStream(videoHandle * in_Handle)
  {
    mHandle = in_Handle;
    
    if(!sRegistered)
    {
      av_register_all();
      sRegistered = true;

#ifndef _WIN32
      printf("\n-----------------------------\n");
      printf("FabricVIDEO supported input formats: ");
      for(AVInputFormat * fmt = first_iformat; fmt != NULL; fmt = fmt->next)
        printf("%s, ",fmt->name);
      printf("\n-----------------------------\n");
      
      printf("FabricVIDEO supported output formats: ");
      for(AVOutputFormat * fmt = first_oformat; fmt != NULL; fmt = fmt->next)
        printf("%s, ",fmt->name);
      printf("\n-----------------------------\n\n");
#endif
    }
    
    mFormatCtx = NULL;
    mCodecCtx = NULL;
    mVideoStreamID = -1;
    mCodec = NULL;
    mFrameYUV = NULL;
    mFrameRGB = NULL;
    mBuffer = NULL;
    mBufferSize = 0;
    mConvertCtx = NULL;
  }
  
  ~videoStream()
  {
    if(mReadOnly)
    {
      av_free(mBuffer);
      av_free(mFrameRGB);
      av_free(mFrameYUV);
      avcodec_close(mCodecCtx);
      av_close_input_file(mFormatCtx);
    }
    else
    {
      av_write_trailer(mFormatCtx);

      avcodec_close(mVideoStream->codec);
      av_free(mFrameRGB->data[0]);
      av_free(mFrameRGB);
      av_free(mFrameYUV->data[0]);
      av_free(mFrameYUV);
      av_free(mBuffer);

      /* free the streams */
      for(size_t i = 0; i < mFormatCtx->nb_streams; i++) {
        av_freep(&mFormatCtx->streams[i]->codec);
        av_freep(&mFormatCtx->streams[i]);
      }
      
      if (!(mFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        /* close the output file */
        url_fclose(mFormatCtx->pb);
      }
      
      /* free the format ctx */
      av_free(mFormatCtx);
    }
  }
  
  KL::Size getWidth()
  {
    return mCodecCtx ? mCodecCtx->width : 0;
  }
  
  KL::Size getHeight()
  {
    return mCodecCtx ? mCodecCtx->height : 0;
  }
  
  KL::Scalar getDuration()
  {
    if(!mReadOnly)
      return 0;
    if(!mFormatCtx)
      return 0;
    return KL::Scalar(mFormatCtx->duration) / KL::Scalar(AV_TIME_BASE);
  }

  KL::Scalar getFPS()
  {
    if(!mReadOnly)
      return 25;
    return KL::Scalar(mFormatCtx->streams[mVideoStreamID]->r_frame_rate.num) / KL::Scalar(mFormatCtx->streams[mVideoStreamID]->r_frame_rate.den);
  }
  
  KL::Scalar getTime()
  {
    if(!mReadOnly)
      return 0;
    return mHandle->time;
  }
  
  bool init(const KL::String & filename, bool in_readOnly, KL::Size width = 0, KL::Size height = 0)
  {
    mReadOnly = in_readOnly;
    if(mReadOnly)
    {
      if(filename.data()== NULL){
        Fabric::EDK::throwException("Video Extension: no filename specified!");
        return false;
      }
      
      // open video file
      if(av_open_input_file(&mFormatCtx, filename.data(), NULL, 0, NULL)!=0)
      {
        Fabric::EDK::throwException("Video Extension: Cannot open video file!");
        return false;
      }
  
      // retrieve stream information
      if(av_find_stream_info(mFormatCtx)<0)
      {
        Fabric::EDK::throwException("Video Extension: Cannot read stream information!");
        return false;
      }
  
      // find the first video stream
      for(int i=0; i<int(mFormatCtx->nb_streams); i++)
      {
        if(mFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
          mVideoStreamID=i;
          break;
        }
      }
      if(mVideoStreamID==-1)
      {
        Fabric::EDK::throwException("Video Extension: Didn't find a video stream in video file!");
        return false;
      }
      
      // get a pointer to the codec context for the video stream
      mCodecCtx = mFormatCtx->streams[mVideoStreamID]->codec;
      
      // find the decoder for the video stream
      mCodec = avcodec_find_decoder(mCodecCtx->codec_id);
      if(mCodec==NULL)
      {
        Fabric::EDK::throwException("Video Extension: Decoder codec not found!");
        return false;
      }
      
      // open codec
      if(avcodec_open(mCodecCtx, mCodec)<0)
      {
        Fabric::EDK::throwException("Video Extension: Could not open supported codec!");
        return false;
      }
      
      // allocate video frame (for YUV and RGB)
      mFrameYUV = avcodec_alloc_frame();
      mFrameRGB = avcodec_alloc_frame();
  
      // determine required buffer size and allocate buffer
      //mBufferSize = avpicture_get_size( PIX_FMT_RGB24, getWidth(),getHeight())*sizeof(uint8_t);
      //mBuffer = (uint8_t *)av_malloc(mBufferSize);
      int numBytes = avpicture_get_size( PIX_FMT_RGB24, getWidth(),getHeight());
      mBuffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
      
      // check if we received the memory
      if(!mFrameRGB || !mFrameYUV || !mBuffer)
      {
        Fabric::EDK::throwException("Video Extension: Could not allocate buffer!");
        return false;
      }
  
      // connect the picture frame with the buffer      
      avpicture_fill((AVPicture *)mFrameRGB, mBuffer, PIX_FMT_RGB24, getWidth(), getHeight());
      
      // init the handle's values
      mHandle->time = -10000.0;
      mHandle->width = getWidth();
      mHandle->height = getHeight();
      mHandle->duration = getDuration();
      mHandle->fps = getFPS();
    }
    else
    {
      AVOutputFormat * fmt = av_guess_format("mpeg2video",NULL,NULL);
      if(!fmt)
      {
        Fabric::EDK::throwException("Video Extension: Format not found!");
        return false;
      }
      
      mFormatCtx = avformat_alloc_context();
      mFormatCtx->oformat = fmt;
      
      snprintf(mFormatCtx->filename, sizeof(mFormatCtx->filename), "%s", filename.data());
      
      mVideoStream = av_new_stream(mFormatCtx, 0);
      mVideoStreamID = 0;

      mCodecCtx = mVideoStream->codec;
      mCodecCtx->codec_id = fmt->video_codec;
      mCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;

      mCodecCtx->bit_rate = 400000;
      mCodecCtx->width = width;
      mCodecCtx->height = height;
      mCodecCtx->time_base.den = 25;
      mCodecCtx->time_base.num = 1;
      mCodecCtx->gop_size = 12; /* emit one intra frame every twelve frames at most */
      mCodecCtx->pix_fmt = PIX_FMT_YUV420P;

      if (mCodecCtx->codec_id == CODEC_ID_MPEG2VIDEO)
        mCodecCtx->max_b_frames = 2;
      if (mCodecCtx->codec_id == CODEC_ID_MPEG1VIDEO)
        mCodecCtx->mb_decision=2;

      if(mFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
      mCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

      if (av_set_parameters(mFormatCtx, NULL) < 0) {
        Fabric::EDK::throwException("Video Extension: Invalid output format parameters!");
        return false;
      }
  
      AVCodec * codec = avcodec_find_encoder(mCodecCtx->codec_id);
      if (!codec) {
        Fabric::EDK::throwException("Video Extension: Encoder codec not found!");
        return false;
      }

      if (avcodec_open(mCodecCtx, codec) < 0) {
        Fabric::EDK::throwException("Video Extension: Could not open codec!");
        return false;
      }

      mBufferSize = avpicture_get_size( PIX_FMT_YUV420P, getWidth(),getHeight())*sizeof(uint8_t);
      mBuffer = (uint8_t *)av_malloc(mBufferSize);

      mFrameRGB = alloc_picture(PIX_FMT_RGB24, getWidth(), getHeight());
      if (!mFrameRGB) {
        Fabric::EDK::throwException("Video Extension: Could not allocate RGB picture!");
        return false;
      }

      mFrameYUV = alloc_picture(PIX_FMT_YUV420P, getWidth(), getHeight());
      if (!mFrameYUV) {
        Fabric::EDK::throwException("Video Extension: Could not allocate YUV picture!");
        return false;
      }
      
      if(!mConvertCtx)
      {
        mConvertCtx = sws_getContext(getWidth(), getHeight(), PIX_FMT_RGB24, getWidth(), getHeight(), PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
        if(!mConvertCtx)
        {
          Fabric::EDK::throwException("Video Extension: Could not create sws_conversion object!");
          return false;
        }
      }

      if (!(fmt->flags & AVFMT_NOFILE)) {
        if (url_fopen(&mFormatCtx->pb, filename.data(), URL_WRONLY) < 0) {
          Fabric::EDK::throwException("Video Extension: Could not open output file!");
          return false;
        }
      }
      
      if(av_write_header(mFormatCtx) < 0)
      {
        Fabric::EDK::throwException("Video Extension: Could not write header!");
        return false;
      }
    }

    return true;
  }
  
  bool readNextFrame(KL::Boolean &loop)
  {
    if(!mReadOnly)
      return false;
    AVPacket packet;
    av_init_packet(&packet);
    int frameFinished = 0;
    bool result = false;
    while(av_read_frame(mFormatCtx, &packet)>=0)
    {
      // is this a packet from the video stream?
      if(packet.stream_index == mVideoStreamID)
      {
        // decode video frame
        avcodec_decode_video2(mCodecCtx, mFrameYUV, &frameFinished, &packet);
        
        // did we get a complete video frame?
        if(frameFinished != 0)
        {
          // let's see if we need to create a conversion context
          if(!mConvertCtx)
          {
            mConvertCtx = sws_getContext(getWidth(), getHeight(), mCodecCtx->pix_fmt, getWidth(), getHeight(), PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
            if(!mConvertCtx)
            {
              Fabric::EDK::throwException("Video Extension: Could not create sws_conversion object!");
              return false;
            }
          }

          // convert the image from its native format to RGB
          sws_scale(mConvertCtx, mFrameYUV->data, mFrameYUV->linesize, 0, getHeight(), mFrameRGB->data, mFrameRGB->linesize);
          result = true;
          break;
        }
      }
      
      // Free the packet that was allocated by av_read_frame
      av_free_packet(&packet);
    }
    
    // if we didn't find a frame (we are at the end)
    if(!result && loop)
      av_seek_frame(mFormatCtx,mVideoStreamID,0,0);
    else
      mHandle->time += 1.0 / getFPS();

    return result;
  }

  bool seekTime(KL::Scalar &time)
  {
    if(!mReadOnly)
      return false;
    KL::Boolean loop = false;

    if(fabs(time - mHandle->time) < cFrameSkipTolerance)
      return true;

    if(fabs(time - mHandle->time) > 1.0 / getFPS() + cFrameSkipTolerance)
    {
      if(fabs(time - mHandle->time) < 2.0 / getFPS() + cFrameSkipTolerance)
      {
        readNextFrame(loop);
      }
      else
      {
        int64_t frame = (int64_t)(0.5 * (time - 1.0 / getFPS()) * KL::Scalar(mCodecCtx->time_base.den) / KL::Scalar(mCodecCtx->time_base.num));
        av_seek_frame(mFormatCtx,mVideoStreamID,frame,0);
        mHandle->time = time - 1.0 / getFPS();
      }
    }
    
    return readNextFrame(loop);
  }
  
  bool getAllPixels(KL::VariableArray<KL::RGB> &pixels)
  {
    if(!mReadOnly)
      return false;
    mHandle->width = getWidth();
    mHandle->height = getHeight();
    pixels.resize(getHeight() * getWidth());
    memcpy(&pixels[0],mFrameRGB->data[0],sizeof(KL::RGB) * getHeight() * getWidth());
    return true;
  }

  bool writeFrame(KL::VariableArray<KL::RGB> &pixels)
  {
    if(mReadOnly)
      return false;
    
    if(pixels.size() != getWidth() * getHeight())
      return false;
    
    memcpy(mFrameRGB->data[0],&pixels[0],sizeof(KL::RGB) * getHeight() * getWidth());

    sws_scale(mConvertCtx, mFrameRGB->data, mFrameRGB->linesize, 0, getHeight(), mFrameYUV->data, mFrameYUV->linesize);

    size_t out_size = avcodec_encode_video(mCodecCtx, mBuffer, mBufferSize, mFrameYUV);
    if (out_size > 0) {
      
      AVPacket pkt;
      av_init_packet(&pkt);
      
      if (mCodecCtx->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(mCodecCtx->coded_frame->pts, mCodecCtx->time_base, mVideoStream->time_base);
      if(mCodecCtx->coded_frame->key_frame)
        pkt.flags |= AV_PKT_FLAG_KEY;
      pkt.stream_index= mVideoStream->index;
      pkt.data = mBuffer;
      pkt.size = out_size;
      
      av_interleaved_write_frame(mFormatCtx, &pkt);
    }
    return true;
  }
  
};
bool videoStream::sRegistered = false;

FABRIC_EXT_EXPORT void FabricVIDEOOpenFileName(
  const KL::String & filename,
  videoHandle &handle
)
{
  if(handle.pointer == NULL)
  {
    // init the stream
    handle.pointer = new videoStream(&handle);
    if(!handle.pointer){
      Fabric::EDK::throwException("Video Extension: Could not create videoStream object!");
      return;
    }
    if(!handle.pointer->init(filename,true))
    {
      delete(handle.pointer);
      handle.pointer = NULL;
      return;
    }
  }
}

FABRIC_EXT_EXPORT void FabricVIDEOOpenResource(
  KL::Data resourceData,
  KL::Size resourceDataSize,
  videoHandle &handle
)
{
  if(handle.pointer == NULL && resourceData != NULL && resourceDataSize > 0)
  {
#if defined(FABRIC_OS_WINDOWS)
    char const *dir = getenv("APPDATA");
    if(dir == NULL)
      dir = getenv("TEMP");
    if(dir == NULL)
      dir = getenv("TMP");
    if(dir == NULL)
      Fabric::EDK::throwException("Alembic extension: environment variable APP_DATA or TMP or TEMP is undefined");
    KL::String fileName( _tempnam( dir, "tmpfab_" ) );
#else
    KL::String fileName(tmpnam(NULL));
#endif

    // save the file to disk
    FILE * file = fopen(fileName.data(),"wb");
    fwrite(resourceData,resourceDataSize,1,file);
    fclose(file);
    file = NULL;
    
    return FabricVIDEOOpenFileName(fileName,handle);
  }
}

FABRIC_EXT_EXPORT void FabricVIDEOOpenFileHandle(
  const KL::String & file,
  videoHandle &handle
)
{
  if(handle.pointer == NULL)
  {
    KL::FileHandleWrapper wrapper(file);
    wrapper.ensureIsValidFile();
    return FabricVIDEOOpenFileName(wrapper.getPath(),handle);
  }
}

FABRIC_EXT_EXPORT void FabricVIDEOSeekTime(
  videoHandle & handle,
  KL::Scalar &time
)
{
  if(handle.pointer == NULL)
    return;
  handle.pointer->seekTime(time);
}

FABRIC_EXT_EXPORT void FabricVIDEOCreateFromFileHandle(
  const KL::String & file,
  KL::Size width,
  KL::Size height,
  videoHandle &handle
)
{
  if(handle.pointer == NULL)
  {
    KL::FileHandleWrapper wrapper(file);
    if(wrapper.isFolder())
    {
      Fabric::EDK::throwException("Video extension: Cannot write to a folder.");
      return;
    }
    if(wrapper.isReadOnly())
    {
      Fabric::EDK::throwException("Video Extension: Provided fileHandle is readOnly, invalid for output video!");
      return;
    }
    wrapper.ensureTargetExists();

    // init the stream
    handle.pointer = new videoStream(&handle);
    if(!handle.pointer){
      Fabric::EDK::throwException("Video Extension: Could not create videoStream object!");
      handle.pointer = NULL;
      return;
    }
    KL::String filePath = wrapper.getPath();
    if(!handle.pointer->init(filePath,false,width,height))
    {
      delete(handle.pointer);
      handle.pointer = NULL;
      return;
    }
    handle.width = handle.pointer->getWidth();
    handle.height = handle.pointer->getHeight();
    handle.duration = 0;
    handle.fps = handle.pointer->getFPS();
  }
}

FABRIC_EXT_EXPORT void FabricVIDEOGetAllPixels(
  videoHandle & handle,
  KL::VariableArray<KL::RGB> &pixels
)
{
  if(handle.pointer == NULL)
    return;
  handle.pointer->getAllPixels(pixels);
}

FABRIC_EXT_EXPORT void FabricVIDEOWriteAllPixels(
  videoHandle & handle,
  KL::VariableArray<KL::RGB> &pixels
)
{
  if(handle.pointer == NULL)
    return;
  handle.pointer->writeFrame(pixels);
  handle.duration += 1.0f / float(handle.pointer->getFPS());
}

FABRIC_EXT_EXPORT void FabricVIDEOFreeHandle(
  videoHandle &handle
)
{
  if(handle.pointer != NULL) {
    delete(handle.pointer);
    handle.pointer = NULL;
  }
}
