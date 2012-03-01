/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "OCL.h"
#include "Debug.h"

#include <Fabric/Core/CG/BooleanAdapter.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/IntegerAdapter.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/RT/BooleanDesc.h>
#include <Fabric/Core/RT/IntegerDesc.h>
#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/RT/VariableArrayDesc.h>
#include <Fabric/Core/RT/VariableArrayImpl.h>
#include <Fabric/Core/RT/OpaqueDesc.h>
#include <Fabric/Core/RT/NumericDesc.h>
#include <Fabric/Core/RT/Manager.h>

#if defined( FABRIC_OS_MACOSX )
# include <OpenCL/opencl.h>
# include <OpenGL/OpenGL.h>
# define CL_CALLBACK
#elif defined( FABRIC_OS_WINDOWS )
# include <windows.h>
# include <CL/opencl.h>
# include <GL/glew.h>
#elif defined( FABRIC_OS_LINUX )
# include <CL/opencl.h>
# include <GL/glx.h>
#else
# error Unsupported platform! 
#endif

// http://www.bealto.com/mp-mandelbrot_simple-opencl.html

namespace Fabric
{
  namespace OCL
  {
    typedef Util::UnorderedMap< std::string, void * > FuncTable;
    static FuncTable  llvmFuncTable;
    
    static RC::ConstHandle<RT::StringDesc> stringDesc;
    static RC::ConstHandle<RT::OpaqueDesc> clPlatformIDDesc;
    static RC::ConstHandle<RT::VariableArrayDesc> clPlatformIDVariableArrayDesc;
    static RC::ConstHandle<RT::OpaqueDesc> clDeviceIDDesc;
    static RC::ConstHandle<RT::VariableArrayDesc> clDeviceIDVariableArrayDesc;
    static RC::ConstHandle<RT::OpaqueDesc> clContextDesc;
    static RC::ConstHandle<RT::OpaqueDesc> clCommandQueueDesc;
    static RC::ConstHandle<RT::OpaqueDesc> clProgramDesc;
    static RC::ConstHandle<RT::OpaqueDesc> clKernelDesc;
    static RC::ConstHandle<RT::OpaqueDesc> clMemDesc;
    static RC::ConstHandle<RT::VariableArrayDesc> clMemVariableArrayDesc;
    static RC::ConstHandle<RT::OpaqueDesc> clEventDesc;
    static RC::ConstHandle<RT::VariableArrayDesc> clEventVariableArrayDesc;
    
    static int32_t GetPlatformIDs( void *clPlatformIDsAsOpaqueArray )
    {
      cl_uint clNumPlatforms;
      cl_int result = clGetPlatformIDs( 0, NULL, &clNumPlatforms );
      if ( result == CL_SUCCESS )
      {
        cl_platform_id *clPlatformIDs = new cl_platform_id[clNumPlatforms];
        clGetPlatformIDs( clNumPlatforms, clPlatformIDs, NULL );
        clPlatformIDVariableArrayDesc->setMembers( clPlatformIDsAsOpaqueArray, clNumPlatforms, clPlatformIDs );
        delete [] clPlatformIDs;
      }
      return result;
    }
        
    static int32_t GetDeviceType( cl_device_id clDevice, cl_device_type *clDeviceType )
    {
      return clGetDeviceInfo( clDevice, CL_DEVICE_TYPE, sizeof(*clDeviceType), clDeviceType, NULL );
    }

    static int32_t GetDeviceIDs( cl_platform_id clPlatformID, size_t clDeviceType, void *clDeviceIDsAsOpaqueArray )
    {
      cl_uint clNumDevices = 0;
      cl_int result = clGetDeviceIDs( clPlatformID, clDeviceType, 0, NULL, &clNumDevices );
      if( result == CL_SUCCESS )
      {
        cl_device_id *clDeviceIDs = new cl_device_id[clNumDevices];
        result = clGetDeviceIDs( clPlatformID, clDeviceType, clNumDevices, clDeviceIDs, &clNumDevices );
        if ( result == CL_SUCCESS )
          clDeviceIDVariableArrayDesc->setMembers( clDeviceIDsAsOpaqueArray, clNumDevices, clDeviceIDs );
        delete [] clDeviceIDs;
      }
      
      if( result != CL_SUCCESS ) 
        clDeviceIDVariableArrayDesc->setNumMembers( clDeviceIDsAsOpaqueArray, 0 );

      return result;
    }
    
    static void CL_CALLBACK ContextNotifyCallback( char const *errinfo, void const *private_info, size_t cb, void *user_data )
    {
      FABRIC_DEBUG_LOG( "OpenCL context notify: %s\n", errinfo );
      std::string err( errinfo );
      MT::tlsLogCollector.get()->add( "OpenCL error: " + err );
    }
    
    static cl_context CreateContext( void const * const clDeviceIDsRValue, int32_t *clErrCode )
    {
      cl_uint num_devices = clDeviceIDVariableArrayDesc->getNumMembers( &clDeviceIDsRValue );
      cl_device_id const *devices = (cl_device_id const *)clDeviceIDVariableArrayDesc->getImmutableMemberData( &clDeviceIDsRValue, 0 );
      cl_int errcode;
      // [pzion 20110711] FIXME: workaround for OpenGL/OpenCL interop
      cl_context result = clCreateContext( 0, num_devices, devices, &ContextNotifyCallback, NULL, &errcode );
      if ( clErrCode )
        *clErrCode = errcode;
      return result;
    }
    
    static cl_context CreateContext_GL( void const * const clDeviceIDsRValue, int32_t *clErrCode )
    {
#if defined( FABRIC_OS_WINDOWS ) || defined( FABRIC_OS_LINUX )
      std::vector<cl_platform_id> platforms;
      cl_uint numPlatforms;
      clGetPlatformIDs( 0, NULL, &numPlatforms );
      platforms.resize( numPlatforms );
      clGetPlatformIDs( numPlatforms, &platforms[0], NULL );
#endif
#if defined( FABRIC_OS_MACOSX )
      CGLContextObj kCGLContext = CGLGetCurrentContext();
      CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
      cl_context_properties props[] =
      {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup,
        0
      };
#elif defined( FABRIC_OS_WINDOWS )

      cl_uint num_devices = clDeviceIDVariableArrayDesc->getNumMembers( &clDeviceIDsRValue );
      cl_device_id const *devices = (cl_device_id const *)clDeviceIDVariableArrayDesc->getImmutableMemberData( &clDeviceIDsRValue, 0 );

      cl_context_properties props[] = 
      {
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(), 
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(), 
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0]),
        0
      };
#elif defined( FABRIC_OS_LINUX )
      cl_context_properties props[] = 
      {
        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), 
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), 
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0]),
        0
      };
#else
# error Unsupported platform!
#endif
      
      cl_int errcode;

#if defined( FABRIC_OS_WINDOWS )    
      cl_context result = clCreateContext( props, num_devices, devices, &ContextNotifyCallback, NULL, &errcode );
#else
      cl_context result = clCreateContext( props, 0, 0, &ContextNotifyCallback, NULL, &errcode );
#endif
      if ( clErrCode )
        *clErrCode = errcode;
      return result;
    }
        
    static int32_t GetContextDevices( cl_context clContext, void *clDeviceIDsAsOpaqueArray )
    {
      size_t clNumDevices = 0;
      cl_int result = clGetContextInfo( clContext, CL_CONTEXT_DEVICES, 0, NULL, &clNumDevices );
      if( result == CL_SUCCESS )
      {
        cl_device_id *clDeviceIDs = new cl_device_id[clNumDevices];
        result = clGetContextInfo( clContext, CL_CONTEXT_DEVICES, clNumDevices, clDeviceIDs, &clNumDevices );
        if ( result == CL_SUCCESS )
          clDeviceIDVariableArrayDesc->setMembers( clDeviceIDsAsOpaqueArray, clNumDevices, clDeviceIDs );
        delete [] clDeviceIDs;
      }
      return result;
    }

    static cl_command_queue CreateCommandQueue( cl_context clContext, cl_device_id clDeviceID, size_t clCommandQueueProperties, int32_t *clErrCode )
    {
      cl_int errcode;
      cl_command_queue result = clCreateCommandQueue( clContext, clDeviceID, clCommandQueueProperties, &errcode );
      if ( clErrCode )
        *clErrCode = errcode;
      return result;
    }
    
    static cl_program CreateProgramWithSource( cl_context clContext, void const * const sourceStringRValue, int32_t *clErrCode )
    {
      cl_uint count = 1;
      char const *sources[1] = { stringDesc->getValueData( &sourceStringRValue ) };
      size_t lengths[1] = { stringDesc->getValueLength( &sourceStringRValue ) };
      cl_int errcode;
      cl_program result = clCreateProgramWithSource( clContext, count, sources, lengths, &errcode );
      if ( clErrCode )
        *clErrCode = errcode;
      return result;
    }
    
    static int32_t BuildProgram( cl_program clProgram, void const * const clDeviceIDsRValue, void const * const optionsStringRValue )
    {
      cl_uint num_devices = clDeviceIDVariableArrayDesc->getNumMembers( &clDeviceIDsRValue );
      cl_device_id const *devices = (cl_device_id const *)clDeviceIDVariableArrayDesc->getImmutableMemberData( &clDeviceIDsRValue, 0 );
      char const *options = stringDesc->getValueData( &optionsStringRValue );
      cl_int result = clBuildProgram( clProgram, num_devices, devices, options, NULL, NULL );
      return result;
    }
    
    static int32_t GetMemObjectSize( cl_mem clMem, size_t *size )
    {
      return clGetMemObjectInfo( clMem, CL_MEM_SIZE, sizeof(*size), size, NULL );
    }
    
    static int32_t GetProgramBuildInfoi( cl_program program, cl_device_id device, size_t param_name, int32_t *retval )
    {
      if( param_name != CL_PROGRAM_BUILD_STATUS )
        return CL_INVALID_VALUE;
      
      return clGetProgramBuildInfo( program, device, param_name, sizeof( *retval ), retval, NULL );
    }
    
    static int32_t GetProgramBuildInfoStr( cl_program program, cl_device_id device, int32_t param_name, void *stringData )
    {
      if( param_name != CL_PROGRAM_BUILD_OPTIONS && param_name != CL_PROGRAM_BUILD_LOG )
        return CL_INVALID_VALUE;
      
      char *info = new char[ 1024 ];
      size_t nbCopied = 0;
      
      int32_t err = clGetProgramBuildInfo( program, device, param_name, 1024, info, &nbCopied );
      stringDesc->setValue( info, nbCopied, stringData );
      return err;
    }

    static cl_kernel CreateKernel( cl_program clProgram, void const * const kernelNameStringRValue, int32_t *clErrCode )
    {
      char const *kernel_name = stringDesc->getValueData( &kernelNameStringRValue );
      cl_int errcode;
      cl_kernel result = clCreateKernel( clProgram, kernel_name, &errcode );
      if ( clErrCode )
        *clErrCode = errcode;
      return result;
    }
    
    static cl_mem CreateBuffer( cl_context clContext, size_t flags, size_t size, void *host_ptr, int32_t *clErrCode )
    {
      cl_int errcode;
      cl_mem result = clCreateBuffer( clContext, flags, size, host_ptr, &errcode );
      if ( clErrCode )
        *clErrCode = errcode;
      return result;
    }

    static cl_mem CreateFromGLBuffer( cl_context clContext, size_t flags, GLuint bufobj, int32_t *clErrCode )
    {
      cl_int errcode;
      cl_mem result = clCreateFromGLBuffer( clContext, flags, bufobj, &errcode );
      if ( clErrCode )
        *clErrCode = errcode;
      return result;
    }
    
    static int32_t ReleaseMemObject( cl_mem memobj )
    {
      cl_int result = clReleaseMemObject( memobj );
      return result;
    }
    
    static int32_t EnqueueReadBuffer(
      cl_command_queue command_queue,
      cl_mem buffer,
      bool blocking_read,
      size_t offset, 
      size_t cb, 
      void *ptr, 
      void const * const eventWaitListArrayRValue, 
      cl_event *event 
      )
    {
      cl_uint num_events_in_wait_list = clEventVariableArrayDesc->getNumMembers( &eventWaitListArrayRValue );
      cl_event const *event_wait_list = num_events_in_wait_list? (cl_event const *)clEventVariableArrayDesc->getImmutableMemberData( &eventWaitListArrayRValue, 0 ): NULL;
      cl_int result = clEnqueueReadBuffer( command_queue, buffer, blocking_read? CL_TRUE: CL_FALSE, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event );
      return result;
    }

    static int32_t EnqueueAcquireGLObjects(
      cl_command_queue command_queue,
      void const * const memObjectsArrayRValue, 
      void const * const eventWaitListArrayRValue, 
      cl_event *event
      )
    {
      cl_uint num_mem_objects = clMemVariableArrayDesc->getNumMembers( &memObjectsArrayRValue );
      cl_mem const *mem_objects = num_mem_objects? (cl_mem const *)clMemVariableArrayDesc->getImmutableMemberData( &memObjectsArrayRValue, 0 ): NULL;

      cl_uint num_events_in_wait_list = clEventVariableArrayDesc->getNumMembers( &eventWaitListArrayRValue );
      cl_event const *event_wait_list = num_events_in_wait_list? (cl_event const *)clEventVariableArrayDesc->getImmutableMemberData( &eventWaitListArrayRValue, 0 ): NULL;

      cl_int result = clEnqueueAcquireGLObjects( command_queue, num_mem_objects, mem_objects, num_events_in_wait_list, event_wait_list, event );
      return result;
    }

    static int32_t EnqueueReleaseGLObjects(
      cl_command_queue command_queue,
      void const * const memObjectsArrayRValue, 
      void const * const eventWaitListArrayRValue, 
      cl_event *event
      )
    {
      cl_uint num_mem_objects = clMemVariableArrayDesc->getNumMembers( &memObjectsArrayRValue );
      cl_mem const *mem_objects = num_mem_objects? (cl_mem const *)clMemVariableArrayDesc->getImmutableMemberData( &memObjectsArrayRValue, 0 ): NULL;

      cl_uint num_events_in_wait_list = clEventVariableArrayDesc->getNumMembers( &eventWaitListArrayRValue );
      cl_event const *event_wait_list = num_events_in_wait_list? (cl_event const *)clEventVariableArrayDesc->getImmutableMemberData( &eventWaitListArrayRValue, 0 ): NULL;

      cl_int result = clEnqueueReleaseGLObjects( command_queue, num_mem_objects, mem_objects, num_events_in_wait_list, event_wait_list, event );
      return result;
    }

    static int32_t EnqueueWriteBuffer( cl_command_queue command_queue, cl_mem buffer, bool blocking_write, size_t offset, size_t cb, void const *ptr, void const * const eventWaitListArrayRValue, cl_event *event )
    {
      cl_uint num_events_in_wait_list = clEventVariableArrayDesc->getNumMembers( &eventWaitListArrayRValue );
      cl_event const *event_wait_list = num_events_in_wait_list? (cl_event const *)clEventVariableArrayDesc->getImmutableMemberData( &eventWaitListArrayRValue, 0 ): NULL;
      cl_int result = clEnqueueWriteBuffer( command_queue, buffer, blocking_write? CL_TRUE: CL_FALSE, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event );
      return result;
    }
    
    static int32_t SetKernelArg( cl_kernel kernel, size_t arg_index, size_t arg_size, void const *arg_value )
    {
      cl_int result = clSetKernelArg( kernel, arg_index, arg_size, arg_value );
      return result;
    }
    
    static int32_t GetKernelWorkGroupInfo( cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name, size_t param_value_size, void *param_value, size_t *param_value_size_ret )
    {
      cl_int result = clGetKernelWorkGroupInfo( kernel, device, param_name, param_value_size, param_value, param_value_size_ret );
      return result;
    }
    
    static int32_t EnqueueNDRangeKernel(
      cl_command_queue command_queue,
      cl_kernel kernel,
      size_t work_dim,
      size_t const *global_work_offset,
      size_t const *global_work_size,
      size_t const *local_work_size,
      void const * const eventWaitListArrayRValue,
      cl_event *event
      )
    {
      cl_uint num_events_in_wait_list = clEventVariableArrayDesc->getNumMembers( &eventWaitListArrayRValue );
      cl_event const *event_wait_list = num_events_in_wait_list? (cl_event const *)clEventVariableArrayDesc->getImmutableMemberData( &eventWaitListArrayRValue, 0 ): NULL;
      cl_int result = clEnqueueNDRangeKernel(
        command_queue,
        kernel,
        work_dim,
        global_work_offset,
        global_work_size,
        local_work_size,
        num_events_in_wait_list,
        event_wait_list,
        event
        );
      return result;
    }
    
    static int32_t Finish( cl_command_queue command_queue )
    {
      cl_int result = clFinish( command_queue );
      return result;
    }

    void registerTypes( RC::Handle<RT::Manager> const &rtManager )
    {
      //printf("registering OCL....\n");
      
      RC::ConstHandle<RT::BooleanDesc> booleanDesc = rtManager->getBooleanDesc();
      RC::ConstHandle<RT::IntegerDesc> integerDesc = rtManager->getIntegerDesc();
      RC::ConstHandle<RT::SizeDesc> sizeDesc = rtManager->getSizeDesc();
      stringDesc = rtManager->getStringDesc();

      rtManager->registerAlias( "cl_bool", booleanDesc );
      rtManager->registerAlias( "cl_int", integerDesc );
      rtManager->registerAlias( "cl_device_type", sizeDesc );
      rtManager->registerAlias( "cl_context_info", sizeDesc );
      rtManager->registerAlias( "cl_command_queue_properties", sizeDesc );
      rtManager->registerAlias( "cl_mem_flags", sizeDesc );
      rtManager->registerAlias( "cl_kernel_work_group_info", integerDesc );
      rtManager->registerAlias( "cl_program_build_info", integerDesc );
      
      
      clPlatformIDDesc = rtManager->registerOpaque( "cl_platform_id", sizeof(cl_platform_id) );
      clPlatformIDVariableArrayDesc = rtManager->getVariableArrayOf( clPlatformIDDesc, RT::VariableArrayImpl::FLAG_COPY_ON_WRITE );
      clDeviceIDDesc = rtManager->registerOpaque( "cl_device_id", sizeof(cl_device_id) );
      clDeviceIDVariableArrayDesc = rtManager->getVariableArrayOf( clDeviceIDDesc, RT::VariableArrayImpl::FLAG_COPY_ON_WRITE );
      clContextDesc = rtManager->registerOpaque( "cl_context", sizeof(cl_context) );
      clCommandQueueDesc = rtManager->registerOpaque( "cl_command_queue", sizeof(cl_command_queue) );
      clProgramDesc = rtManager->registerOpaque( "cl_program", sizeof(cl_program) );
      clKernelDesc = rtManager->registerOpaque( "cl_kernel", sizeof(cl_kernel) );
      clMemDesc = rtManager->registerOpaque( "cl_mem", sizeof(cl_mem) );
      clMemVariableArrayDesc = rtManager->getVariableArrayOf( clMemDesc, RT::VariableArrayImpl::FLAG_COPY_ON_WRITE );
      clEventDesc = rtManager->registerOpaque( "cl_event", sizeof(cl_event) );
      clEventVariableArrayDesc = rtManager->getVariableArrayOf( clEventDesc, RT::VariableArrayImpl::FLAG_COPY_ON_WRITE );
    }

#define ADD_CONST_BOOL( name, value ) \
  moduleBuilder.getScope().put( name, CG::ConstantSymbol::Create( CG::ExprValue( booleanAdapter, CG::USAGE_RVALUE, moduleBuilder.getContext(), booleanAdapter->llvmConst( moduleBuilder.getContext(), value ) ) ) )
#define ADD_CONST_INT( name ) \
  moduleBuilder.getScope().put( #name, CG::ConstantSymbol::Create( CG::ExprValue( integerAdapter, CG::USAGE_RVALUE, moduleBuilder.getContext(), integerAdapter->llvmConst( moduleBuilder.getContext(), name ) ) ) )
#define ADD_CONST_SIZE( name ) \
  moduleBuilder.getScope().put( #name, CG::ConstantSymbol::Create( CG::ExprValue( sizeAdapter, CG::USAGE_RVALUE, moduleBuilder.getContext(), sizeAdapter->llvmConst( moduleBuilder.getContext(), name ) ) ) )
#define ADD_FUNC_QUOTE( x ) #x
#define ADD_FUNC( name, paramLayout ) \
  do { \
    std::string friendlyEntryName = ADD_FUNC_QUOTE(cl##name); \
    CG::FunctionBuilder( moduleBuilder, friendlyEntryName, paramLayout, true, &friendlyEntryName ); \
    llvmFuncTable[friendlyEntryName] = (void *)&name; \
  } while ( false )
    
    void llvmPrepareModule( CG::ModuleBuilder &moduleBuilder, RC::Handle<RT::Manager> const &rtManager )
    {
      llvmFuncTable.clear();
      
      RC::Handle<CG::Manager> cgManager = moduleBuilder.getManager();
      registerTypes( rtManager );
      
      RC::ConstHandle<CG::BooleanAdapter> booleanAdapter = cgManager->getBooleanAdapter();
      RC::ConstHandle<CG::IntegerAdapter> integerAdapter = cgManager->getIntegerAdapter();
      RC::ConstHandle<CG::SizeAdapter> sizeAdapter = cgManager->getSizeAdapter();
      
      ADD_CONST_BOOL( "CL_FALSE", false );
      ADD_CONST_BOOL( "CL_TRUE", true );
      
      // OpenCL error codes
      ADD_CONST_INT( CL_SUCCESS );
      ADD_CONST_INT( CL_DEVICE_NOT_FOUND );
      ADD_CONST_INT( CL_DEVICE_NOT_AVAILABLE );
      ADD_CONST_INT( CL_COMPILER_NOT_AVAILABLE );
      ADD_CONST_INT( CL_MEM_OBJECT_ALLOCATION_FAILURE );
      ADD_CONST_INT( CL_OUT_OF_RESOURCES );
      ADD_CONST_INT( CL_OUT_OF_HOST_MEMORY );
      ADD_CONST_INT( CL_PROFILING_INFO_NOT_AVAILABLE );
      ADD_CONST_INT( CL_MEM_COPY_OVERLAP );
      ADD_CONST_INT( CL_IMAGE_FORMAT_MISMATCH );
      ADD_CONST_INT( CL_IMAGE_FORMAT_NOT_SUPPORTED );
      ADD_CONST_INT( CL_BUILD_PROGRAM_FAILURE );
      ADD_CONST_INT( CL_MAP_FAILURE );
      ADD_CONST_INT( CL_INVALID_VALUE );
      ADD_CONST_INT( CL_INVALID_DEVICE_TYPE );
      ADD_CONST_INT( CL_INVALID_PLATFORM );
      ADD_CONST_INT( CL_INVALID_DEVICE );
      ADD_CONST_INT( CL_INVALID_CONTEXT );
      ADD_CONST_INT( CL_INVALID_QUEUE_PROPERTIES );
      ADD_CONST_INT( CL_INVALID_COMMAND_QUEUE );
      ADD_CONST_INT( CL_INVALID_HOST_PTR );
      ADD_CONST_INT( CL_INVALID_MEM_OBJECT );
      ADD_CONST_INT( CL_INVALID_IMAGE_FORMAT_DESCRIPTOR );
      ADD_CONST_INT( CL_INVALID_IMAGE_SIZE );
      ADD_CONST_INT( CL_INVALID_SAMPLER );
      ADD_CONST_INT( CL_INVALID_BINARY );
      ADD_CONST_INT( CL_INVALID_BUILD_OPTIONS );
      ADD_CONST_INT( CL_INVALID_PROGRAM );
      ADD_CONST_INT( CL_INVALID_PROGRAM_EXECUTABLE );
      ADD_CONST_INT( CL_INVALID_KERNEL_NAME );
      ADD_CONST_INT( CL_INVALID_KERNEL_DEFINITION );
      ADD_CONST_INT( CL_INVALID_KERNEL );
      ADD_CONST_INT( CL_INVALID_ARG_INDEX );
      ADD_CONST_INT( CL_INVALID_ARG_VALUE );
      ADD_CONST_INT( CL_INVALID_ARG_SIZE );
      ADD_CONST_INT( CL_INVALID_KERNEL_ARGS );
      ADD_CONST_INT( CL_INVALID_WORK_DIMENSION );
      ADD_CONST_INT( CL_INVALID_WORK_GROUP_SIZE );
      ADD_CONST_INT( CL_INVALID_WORK_ITEM_SIZE );
      ADD_CONST_INT( CL_INVALID_GLOBAL_OFFSET );
      ADD_CONST_INT( CL_INVALID_EVENT_WAIT_LIST );
      ADD_CONST_INT( CL_INVALID_EVENT );
      ADD_CONST_INT( CL_INVALID_OPERATION );
      ADD_CONST_INT( CL_INVALID_GL_OBJECT );
      ADD_CONST_INT( CL_INVALID_BUFFER_SIZE );
      ADD_CONST_INT( CL_INVALID_MIP_LEVEL );
      ADD_CONST_INT( CL_INVALID_GLOBAL_WORK_SIZE );
      
      // cl_device_type
      ADD_CONST_SIZE( CL_DEVICE_TYPE_CPU );
      ADD_CONST_SIZE( CL_DEVICE_TYPE_GPU );
      ADD_CONST_SIZE( CL_DEVICE_TYPE_ACCELERATOR );
      ADD_CONST_SIZE( CL_DEVICE_TYPE_DEFAULT );
      ADD_CONST_SIZE( CL_DEVICE_TYPE_ALL );
      
      // cl_command_queue_properties
      ADD_CONST_SIZE( CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE );
      ADD_CONST_SIZE( CL_QUEUE_PROFILING_ENABLE );
      
      // cl_mem_flags
      ADD_CONST_SIZE( CL_MEM_READ_WRITE );
      ADD_CONST_SIZE( CL_MEM_WRITE_ONLY );
      ADD_CONST_SIZE( CL_MEM_READ_ONLY );
      ADD_CONST_SIZE( CL_MEM_USE_HOST_PTR );
      ADD_CONST_SIZE( CL_MEM_ALLOC_HOST_PTR );
      ADD_CONST_SIZE( CL_MEM_COPY_HOST_PTR );
      
      // cl_kernel_work_group_info
      ADD_CONST_INT( CL_KERNEL_WORK_GROUP_SIZE );
      ADD_CONST_INT( CL_KERNEL_COMPILE_WORK_GROUP_SIZE );
      ADD_CONST_INT( CL_KERNEL_LOCAL_MEM_SIZE );
      
      // cl_program_build_info
      ADD_CONST_INT( CL_PROGRAM_BUILD_STATUS );
      ADD_CONST_INT( CL_PROGRAM_BUILD_OPTIONS );
      ADD_CONST_INT( CL_PROGRAM_BUILD_LOG );
      
      //
      ADD_FUNC( GetPlatformIDs, "=Integer,>cl_platform_id[] clPlatformIDs" );
      ADD_FUNC( GetDeviceIDs, "=Integer,<cl_platform_id clPlatformID,<cl_device_type clDeviceType,>cl_device_id[] clDeviceIDs" );
      ADD_FUNC( CreateContext, "=cl_context,<cl_device_id[] clDeviceIDs,>Integer clErrCode" );
      ADD_FUNC( CreateContext_GL, "=cl_context,<cl_device_id[] clDeviceIDs,>Integer clErrCode" );
      ADD_FUNC( GetContextDevices, "=Integer,<cl_context clContext,>cl_device_id[] clDeviceIDs" );
      ADD_FUNC( CreateCommandQueue, "=cl_command_queue,<cl_context clContext,<cl_device_id clDeviceID,<cl_command_queue_properties clCommandQueueProperties,>Integer clErrCode" );
      ADD_FUNC( CreateProgramWithSource, "=cl_program,<cl_context clContext,<String string,>Integer clErrCode" );
      ADD_FUNC( BuildProgram, "=Integer,<cl_program clProgram,<cl_device_id[] clDeviceIDs,<String options" );
      ADD_FUNC( CreateKernel, "=cl_kernel,<cl_program clProgram,<String kernelName,>Integer clErrCode" );
      ADD_FUNC( CreateBuffer, "=cl_mem,<cl_context clContext,<cl_mem_flags clMemFlags,<Size size,<Data host_ptr,>Integer clErrCode" );
      ADD_FUNC( CreateFromGLBuffer, "=cl_mem,<cl_context clContext,<cl_mem_flags clMemFlags,<Integer bufobj,>Integer clErrCode" );
      ADD_FUNC( ReleaseMemObject, "=Integer,<cl_mem memobj" );
      ADD_FUNC( EnqueueReadBuffer, "=Integer,<cl_command_queue command_queue,<cl_mem buffer,<Boolean blocking_read,<Size offset,<Size cb,<Data ptr,<cl_event[] clEventArray,>cl_event event" );
      ADD_FUNC( EnqueueWriteBuffer, "=Integer,<cl_command_queue command_queue,<cl_mem buffer,<Boolean blocking_read,<Size offset,<Size cb,<Data ptr,<cl_event[] clEventArray,>cl_event event" );
      ADD_FUNC( EnqueueAcquireGLObjects, "=Integer,<cl_command_queue command_queue,<cl_mem[] clMemArray,<cl_event[] clEventArray,>cl_event event" );
      ADD_FUNC( EnqueueReleaseGLObjects, "=Integer,<cl_command_queue command_queue,<cl_mem[] clMemArray,<cl_event[] clEventArray,>cl_event event" );
      ADD_FUNC( SetKernelArg, "=Integer,<cl_kernel kernel,<Size arg_index,<Size arg_size,<Data arg_value" );
      ADD_FUNC( GetKernelWorkGroupInfo, "=Integer,<cl_kernel kernel,<cl_device_id device,<cl_kernel_work_group_info param_name,<Size param_value_size,<Data param_value,<Data param_value_size_ret" );
      ADD_FUNC( EnqueueNDRangeKernel, "=Integer,<cl_command_queue command_queue,<cl_kernel kernel,<Size work_dim,<Data global_work_offset,<Data global_work_size,<Data local_work_size,<cl_event[] clEventArray,>cl_event event" );
      ADD_FUNC( Finish, "=Integer,<cl_command_queue command_queue" );
      ADD_FUNC( GetProgramBuildInfoi, "=Integer,<cl_program program,<cl_device_id device,<cl_program_build_info param_name,>Integer retval" );
      ADD_FUNC( GetProgramBuildInfoStr, "=Integer,<cl_program program,<cl_device_id device,<cl_program_build_info param_name,>String retval" );
      ADD_FUNC( GetMemObjectSize, "=Integer,<cl_mem clMem,>Size size" );
      ADD_FUNC( GetDeviceType, "=Integer,<cl_device_id clDevice,>cl_device_type clDeviceType" );
    }
    
    void *llvmResolveExternalFunction( std::string const &name )
    {
      FuncTable::const_iterator   it;
      it = llvmFuncTable.find( name );
      if( it != llvmFuncTable.end() )
        return( it->second );
      
      return 0;
    }
    
  };
};
