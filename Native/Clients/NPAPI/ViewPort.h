/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_VIEW_PORT_H
#define _FABRIC_NPAPI_VIEW_PORT_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/Util/UnorderedMap.h>
#include <Fabric/Base/Util/SimpleString.h>

#include <GL/glew.h>
#include <npapi/npapi.h>
#include <npapi/npruntime.h>
#if defined( FABRIC_POSIX )
# include <sys/time.h>
#elif defined( FABRIC_WIN32 )
# include <windows.h>
#if defined( interface )
# undef interface
#endif
#include <GL/glew.h>
#endif
#include <vector>
#include <set>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace JSON
  {
    class ArrayEncoder;
    class Encoder;
    struct Entity;
  };
  
  namespace IO
  {
    class Dir;
  };
  
  namespace MT
  {
    class LogCollector;
  };
  
  namespace RT
  {
    class IntegerDesc;
  };
  
  namespace DG
  {
    class Context;
    class Node;
    class Event;
    class LogCollector;
  };
  
  namespace NPAPI
  {
    class Context;
    class Interface;
    
    class ViewPort : public RC::Object
    {
      friend class Interface;

    public:
      REPORT_RC_LEAKS
      
      virtual void getWindowSize( size_t &width, size_t &height ) const = 0;
    
      RC::Handle<DG::Node> getWindowNode() const;
      RC::Handle<DG::Event> getRedrawEvent() const;
      
      float getFPS() const
      {
        return m_fps;
      }
    
      void setRedrawFinishedCallback( NPObject *npObject );
      virtual void needsRedraw() = 0;
      
      virtual NPError nppGetValue( NPPVariable variable, void *value );
      virtual NPError nppSetWindow( NPWindow *npWindow );
      virtual int16_t nppHandleEvent( void *event );
      virtual NPError nppDestroy( NPSavedData** save );
      
      RC::Handle<MT::LogCollector> getLogCollector() const;
      virtual void redrawFinished();
      
      virtual void jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecAddPopupItem( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      virtual void jsonDesc( JSON::Encoder &resultEncoder ) const;
      void jsonNotify( char const *cmdData, size_t length, Util::SimpleString const *arg ) const;
      void jsonNotifyPopUpItem( Util::SimpleString const &arg ) const;

      virtual void pushOGLContext() = 0;
      virtual void popOGLContext() = 0;

      void drawWatermark( size_t width, size_t height );

      virtual std::string queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension ) = 0;

    protected:
    
      ViewPort( RC::ConstHandle<Interface> const &interface );
      ~ViewPort();
      
      RC::ConstHandle<Interface> getInterface() const;
      
      virtual void didResize( size_t width, size_t height ); 
    
      virtual void asyncRedrawFinished();

      struct PopUpItem
      {
        Util::SimpleString desc;
        Util::SimpleString argJSON;
      };
      typedef std::vector<PopUpItem> PopUpItems;

      PopUpItems m_popUpItems;

    private:
    
      void jsonExecGetFPS( JSON::ArrayEncoder &resultArrayEncoder ) const;

      static void AsyncRedrawFinished( void *_this );
          
      NPP m_npp;
      std::string m_name;
      Interface const *m_interface;
      RC::Handle<Context> m_context;
      
      RC::Handle<MT::LogCollector> m_logCollector;
      
      RC::ConstHandle<RT::IntegerDesc> m_integerDesc;
      RC::Handle<DG::Node> m_windowNode;
      RC::Handle<DG::Event> m_redrawEvent;

      NPObject *m_redrawFinishedCallback;
#if defined(FABRIC_OS_WINDOWS)
      LARGE_INTEGER m_fpsStart;
      double        m_fpsTimerFreq;
#else  
      struct timeval m_fpsStart;
#endif
      
      unsigned m_fpsCount; 
      float m_fps;
      
      GLuint m_watermarkShaderProgram;
      GLuint m_watermarkTextureBuffer;
      GLuint m_watermarkPositionsBufferID;
      bool m_watermarkNeedPositionsVBOUpload;
      GLuint m_watermarkUVsBufferID;
      GLuint m_watermarkIndexesBufferID;
      size_t m_watermarkLastWidth;
      size_t m_watermarkLastHeight;

      static std::set<ViewPort*> s_nppAliveViewports;
    };
  };
};

#endif //_FABRIC_NPAPI_VIEW_PORT_H
