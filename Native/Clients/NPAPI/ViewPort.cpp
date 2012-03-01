/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ViewPort.h"
#include "Interface.h"
#include "Watermark.h"
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/IntegerDesc.h>
#include <Fabric/Clients/NPAPI/Context.h>
#include <Fabric/Core/DG/Node.h>
#include <Fabric/Core/DG/Event.h>
#include <Fabric/Core/MT/LogCollector.h>

#if defined(FABRIC_OS_WINDOWS)
# include <intrin.h>
#endif

namespace Fabric
{
  namespace NPAPI
  {
    std::set<ViewPort*> ViewPort::s_nppAliveViewports;

    ViewPort::ViewPort( RC::ConstHandle<Interface> const &interface )
      : m_npp( interface->getNPP() )
      , m_name( "viewPort" )
      , m_interface( interface.ptr() )
      , m_context( interface->getContext() )
      , m_logCollector( m_context->getLogCollector() )
      , m_redrawFinishedCallback( 0 )
      , m_fpsCount( 0 )
      , m_fps( 0.0 )
      , m_watermarkShaderProgram( 0 )
      , m_watermarkTextureBuffer( 0 )
      , m_watermarkPositionsBufferID( 0 )
      , m_watermarkUVsBufferID( 0 )
      , m_watermarkIndexesBufferID( 0 )
      , m_watermarkLastWidth( 0 )
      , m_watermarkLastHeight( 0 )
    {
      s_nppAliveViewports.insert(this);

      RC::Handle<RT::Manager> rtManager = m_context->getRTManager();
      m_integerDesc = rtManager->getIntegerDesc();
      
      m_windowNode = DG::Node::Create( "FABRIC.window", m_context );
      m_windowNode->addMember( "width", m_integerDesc, m_integerDesc->getDefaultData() );
      m_windowNode->addMember( "height", m_integerDesc, m_integerDesc->getDefaultData() );
       
      m_redrawEvent = DG::Event::Create( "FABRIC.window.redraw", m_context );

#if defined(FABRIC_OS_WINDOWS)
      LARGE_INTEGER fpsTimerFreq;
      ::QueryPerformanceFrequency( &fpsTimerFreq );
      m_fpsTimerFreq = double( fpsTimerFreq.QuadPart );
      ::QueryPerformanceCounter( &m_fpsStart );
#else
      gettimeofday( &m_fpsStart, NULL );
#endif

      m_context->registerViewPort( m_name, this );
    }
    
    ViewPort::~ViewPort()
    {
      //[JCG 20111911] This might not be required (done in ViewPort::nppDestroy too), but I'm playing safe to be sure we are compatible with the old retain/release behaviour
      s_nppAliveViewports.erase(this);

      m_context->unregisterViewPort( m_name, this );

      if ( m_redrawFinishedCallback )
        NPN_ReleaseObject( m_redrawFinishedCallback );
    }
    
    void ViewPort::jsonNotify( char const *cmdData, size_t cmdLength, Util::SimpleString const *arg ) const
    {
      std::vector<std::string> src;
      src.push_back("VP");
      src.push_back("viewPort");
      
      m_context->jsonNotify( src, cmdData, cmdLength, arg );
    }
    
    void ViewPort::asyncRedrawFinished()
    {
      jsonNotify( "redrawFinished", 14, 0 );
    }

    RC::Handle<MT::LogCollector> ViewPort::getLogCollector() const
    {
      return m_logCollector;
    }
      
    void ViewPort::redrawFinished()
    {
      ++m_fpsCount;
       
      double secDiff;

#if defined(FABRIC_OS_WINDOWS)
      LARGE_INTEGER    fpsNow;
      ::QueryPerformanceCounter( &fpsNow );

      secDiff = double(fpsNow.QuadPart - m_fpsStart.QuadPart) / m_fpsTimerFreq;
#else
      struct timeval fpsNow;
      gettimeofday( &fpsNow, NULL );
      secDiff = double((int)fpsNow.tv_sec - (int)m_fpsStart.tv_sec) + (double((int)fpsNow.tv_usec - (int)m_fpsStart.tv_usec) / 1.0e6);
#endif

      if ( secDiff >= 1.0 )
      {
        m_fps = float( m_fpsCount / secDiff );
        char buffer[64];
        sprintf( buffer, "%0.2f fps", m_fps );
        static const bool logFPS = false;
        if ( logFPS )
        {
          if ( m_logCollector )
          {
            m_logCollector->add( buffer );
            m_logCollector->flush();
          }
        }
        else FABRIC_DEBUG_LOG( "%s", buffer );

        memcpy( &m_fpsStart, &fpsNow, sizeof(m_fpsStart) );
        m_fpsCount = 0;
      }
      
      m_logCollector->flush();

      //[JCG 20111911] Don't retain until ViewPort::AsyncRedrawFinished is called back, since in Windows FireFox will 
      // sometimes never call it while destroying the client. Instead we use s_nppAliveViewports as a weak pointer set.
      NPN_PluginThreadAsyncCall( m_npp, &ViewPort::AsyncRedrawFinished, this );
    }
    
    void ViewPort::AsyncRedrawFinished( void *_this )
    {
      ViewPort *viewPort = static_cast<ViewPort *>(_this);

      if(s_nppAliveViewports.find(viewPort) != s_nppAliveViewports.end())
        viewPort->asyncRedrawFinished();
    }
    
    void ViewPort::setRedrawFinishedCallback( NPObject *npObject )
    {
      if ( m_redrawFinishedCallback != npObject )
      {
        if ( m_redrawFinishedCallback )
          NPN_ReleaseObject(m_redrawFinishedCallback);
        m_redrawFinishedCallback = npObject;
        if ( m_redrawFinishedCallback )
          NPN_RetainObject(m_redrawFinishedCallback);
      }
    }
    
    void ViewPort::didResize( size_t width, size_t height )
    {
      m_windowNode->setData( "width", 0, &width );
      m_windowNode->setData( "height", 0, &height );    
    }
    
    NPError ViewPort::nppGetValue( NPPVariable variable, void *value )
    {
      return NPERR_INVALID_PARAM;
    }
 
    NPError ViewPort::nppSetWindow( NPWindow *npWindow )
    {
      return NPERR_NO_ERROR;
    }
    
    int16_t ViewPort::nppHandleEvent( void *event )
    {
      return false;
    }

    NPError ViewPort::nppDestroy( NPSavedData** save )
    {
      s_nppAliveViewports.erase(this);
      return NPERR_NO_ERROR;
    }

    RC::Handle<DG::Node> ViewPort::getWindowNode() const
    {
      return m_windowNode;
    }
    
    RC::Handle<DG::Event> ViewPort::getRedrawEvent() const
    {
      return m_redrawEvent;
    }

    RC::ConstHandle<Interface> ViewPort::getInterface() const
    {
      return m_interface;
    }
    
    void ViewPort::jsonExecGetFPS( JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      resultArrayEncoder.makeElement().makeScalar( m_fps );
    }

    void ViewPort::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      size_t width, height;
      getWindowSize( width, height );
      
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      resultObjectEncoder.makeMember( "fps", 3 ).makeScalar( m_fps );
      resultObjectEncoder.makeMember( "width", 5 ).makeInteger( width );
      resultObjectEncoder.makeMember( "height", 6 ).makeInteger( height );
      resultObjectEncoder.makeMember( "windowNode", 10 ).makeString( m_windowNode->getName() );
      resultObjectEncoder.makeMember( "redrawEvent", 11 ).makeString( m_redrawEvent->getName() );
    }

    void ViewPort::jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      if ( cmd.stringIs( "needsRedraw", 11 ) )
        needsRedraw();
      else if ( cmd.stringIs( "getFPS", 6 ) )
        jsonExecGetFPS( resultArrayEncoder );
      else if ( cmd.stringIs( "addPopUpMenuItem", 16 ) )
        jsonExecAddPopupItem( arg, resultArrayEncoder );
      else throw Exception( "unrecognized command" );
    }
    
    void ViewPort::jsonExecAddPopupItem( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      PopUpItem popUpItem;
      
      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "desc", 4 ) )
          {
            valueEntity.requireString();
            popUpItem.desc = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "arg", 3 ) )
          {
            popUpItem.argJSON = Util::SimpleString( valueEntity.data, valueEntity.length );
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( popUpItem.desc.empty() )
        throw Exception( "missing 'desc'" );
      if ( popUpItem.argJSON.empty() )
        throw Exception( "missing 'arg'" );
        
      m_popUpItems.push_back( popUpItem );
    }
    
    void ViewPort::jsonNotifyPopUpItem( Util::SimpleString const &arg ) const
    {
      jsonNotify( "popUpMenuItemSelected", 21, &arg );
    }

    void ViewPort::drawWatermark( size_t width, size_t height )
    {
      static bool doneInit = false;
      static bool drawWatermark = false;
      if ( !doneInit )
      {
        GLenum glewInitError = glewInit();
	if ( glewInitError != GLEW_OK )
          FABRIC_LOG( "WARNING: glewInit() failed: %s", (char const *)glewGetErrorString( glewInitError ) );
        else if ( !GLEW_VERSION_2_0 )
          FABRIC_LOG( "WARNING: OpenGL 2.0 is not supported, cannot draw watermark" );
        else drawWatermark = true;

        doneInit = true;
      }
      
      if ( !drawWatermark || width == 0 || height == 0 )
        return;
        
      try
      {
        if ( width != m_watermarkLastWidth || height != m_watermarkLastHeight )
        {
          m_watermarkNeedPositionsVBOUpload = true;
          m_watermarkLastWidth = width;
          m_watermarkLastHeight = height;
        }
      
        if ( !m_watermarkShaderProgram )
        {
          GLuint vertexShaderID = glCreateShader( GL_VERTEX_SHADER );
          if ( !vertexShaderID )
            throw Exception( "glCreateShader( GL_VERTEX_SHADER ) failed" );
          static const GLsizei numVertexShaderSources = 1;
          GLchar const *vertexShaderSources[numVertexShaderSources] = {
            "\
attribute vec4 a_position;\n\
attribute vec4 a_texCoord;\n\
void main() {\n\
  gl_TexCoord[0].st = a_texCoord.xy;\n\
  gl_Position = a_position;\n\
}\n\
            "
          };
          GLint vertexShaderSourceLengths[numVertexShaderSources];
          vertexShaderSourceLengths[0] = strlen( vertexShaderSources[0] );
          glShaderSource( vertexShaderID, numVertexShaderSources, vertexShaderSources, vertexShaderSourceLengths );
          glCompileShader( vertexShaderID );
          GLint vertexShaderCompileResult[1];
          glGetShaderiv( vertexShaderID, GL_COMPILE_STATUS, vertexShaderCompileResult );
          if ( !vertexShaderCompileResult[0] )
            throw Exception( "vertex shader compilation failure" );

          GLuint fragmentShaderID = glCreateShader( GL_FRAGMENT_SHADER );
          if ( !fragmentShaderID )
            throw Exception( "glCreateShader( GL_FRAGMENT_SHADER ) failed" );
          static const GLsizei numFragmentShaderSources = 1;
          GLchar const *fragmentShaderSources[numFragmentShaderSources] = {
            "\
uniform sampler2D u_rgbaImage;\n\
void main()\n\
{\n\
  gl_FragColor = texture2D( u_rgbaImage, gl_TexCoord[0].st );\n\
}\n\
            "
          };
          GLint fragmentShaderSourceLengths[numFragmentShaderSources];
          fragmentShaderSourceLengths[0] = strlen( fragmentShaderSources[0] );
          glShaderSource( fragmentShaderID, numFragmentShaderSources, fragmentShaderSources, fragmentShaderSourceLengths );
          glCompileShader( fragmentShaderID );
          GLint fragmentShaderCompileResult[1];
          glGetShaderiv( fragmentShaderID, GL_COMPILE_STATUS, fragmentShaderCompileResult );
          if ( !fragmentShaderCompileResult[0] )
            throw Exception( "fragment shader compilation failure" );
          
          m_watermarkShaderProgram = glCreateProgram();
          if ( !m_watermarkShaderProgram )
            throw Exception( "glCreateProgram() failed" );
          glAttachShader( m_watermarkShaderProgram, vertexShaderID );
          glAttachShader( m_watermarkShaderProgram, fragmentShaderID );
          glLinkProgram( m_watermarkShaderProgram );
          GLint linkResult[1];
          glGetProgramiv( m_watermarkShaderProgram, GL_LINK_STATUS, linkResult );
          if ( !linkResult[0] )
            throw Exception( "link failure" );
            
          glDeleteShader( fragmentShaderID );
          glDeleteShader( vertexShaderID );
        }
        glUseProgram( m_watermarkShaderProgram );
        
        GLint posLocation = glGetAttribLocation( m_watermarkShaderProgram, "a_position" );
        GLint texLocation = glGetAttribLocation( m_watermarkShaderProgram, "a_texCoord" );
        GLint smpLocation = glGetUniformLocation( m_watermarkShaderProgram, "u_rgbaImage" );
        
        glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		
        glEnable( GL_TEXTURE_2D );
        glDisable( GL_DEPTH_TEST );
        glDisable( GL_CULL_FACE );

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable( GL_BLEND );

        glActiveTexture( GL_TEXTURE0 );
        if ( !m_watermarkTextureBuffer )
        {
          glGenTextures( 1, &m_watermarkTextureBuffer );
          if ( !m_watermarkTextureBuffer )
            throw Exception( "glGenTextures() failed" );
          glBindTexture( GL_TEXTURE_2D, m_watermarkTextureBuffer );
    
          glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
          glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, watermarkWidth, watermarkHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, watermarkData );

          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        }
        glUniform1i( smpLocation, 0 );
        glBindTexture( GL_TEXTURE_2D, m_watermarkTextureBuffer );

        if ( !m_watermarkPositionsBufferID )
        {
          m_watermarkNeedPositionsVBOUpload = true;
          glGenBuffers( 1, &m_watermarkPositionsBufferID );
        }
        if ( m_watermarkNeedPositionsVBOUpload )
        {
          static const size_t xMargin = 16, yMargin = 16;
          float xMin = float(xMargin)*2.0/float(width)-1.0;
          float xMax = float(xMargin + watermarkWidth)*2.0/float(width)-1.0;
          float yMin = float(yMargin)*2.0/float(height)-1.0;
          float yMax = float(yMargin + watermarkHeight)*2.0/float(height)-1.0;
          GLfloat p[12] =
          {
            xMin, yMax, 0.0,
            xMax, yMax, 0.0,
            xMax, yMin, 0.0,
            xMin, yMin, 0.0
          };
          glBindBuffer( GL_ARRAY_BUFFER, m_watermarkPositionsBufferID );
          glBufferData( GL_ARRAY_BUFFER, sizeof(p), p, GL_STATIC_DRAW );
          glBindBuffer( GL_ARRAY_BUFFER, 0 );
          m_watermarkNeedPositionsVBOUpload = false;
        }
        glBindBuffer( GL_ARRAY_BUFFER, m_watermarkPositionsBufferID );
        glEnableVertexAttribArray( posLocation );
        glVertexAttribPointer( posLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        
        if ( !m_watermarkUVsBufferID )
        {
          static const GLfloat t[8] =
          {
            0.0, 0.0,
            1.0, 0.0,
            1.0, 1.0,
            0.0, 1.0
          };
          glGenBuffers( 1, &m_watermarkUVsBufferID );
          glBindBuffer( GL_ARRAY_BUFFER, m_watermarkUVsBufferID );
          glBufferData( GL_ARRAY_BUFFER, sizeof(t), t, GL_STATIC_DRAW );
          glBindBuffer( GL_ARRAY_BUFFER, 0 );
        }
        glBindBuffer( GL_ARRAY_BUFFER, m_watermarkUVsBufferID );
        glEnableVertexAttribArray( texLocation );
        glVertexAttribPointer( texLocation, 2, GL_FLOAT, GL_FALSE, 0, NULL );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        
        if ( !m_watermarkIndexesBufferID )
        {
          static const GLuint idx[4] =
          {
            0, 1, 2, 3
          };
          glGenBuffers( 1, &m_watermarkIndexesBufferID );
          glBindBuffer( GL_ARRAY_BUFFER, m_watermarkIndexesBufferID );
          glBufferData( GL_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW );
          glBindBuffer( GL_ARRAY_BUFFER, 0 );
        }
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_watermarkIndexesBufferID );
        glDrawElements( GL_QUADS, 4, GL_UNSIGNED_INT, NULL );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        glBindTexture( GL_TEXTURE_2D, 0 );

        glPopAttrib();
      }
      catch ( Exception e )
      {
        throw "ViewPort::drawWatermark( " + _(width) + ", " + _(height) + " ): " + e;
      }
    }
  };
};
