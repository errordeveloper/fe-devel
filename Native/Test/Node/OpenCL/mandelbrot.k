function entry()
{
  var Size width = 78;
  var Size height = 39;
  var Size count = width * height;
  
  var Integer clErr;

  var cl_platform_id clPlatformID; // NULL by default
  var cl_device_id[] clDeviceIDs;
  clErr = clGetDeviceIDs( clPlatformID, CL_DEVICE_TYPE_GPU, 1, clDeviceIDs );
  var cl_device_id clDeviceID = clDeviceIDs[0];

  var cl_context clContext = clCreateContext( clDeviceIDs, clErr );
  var cl_command_queue clCommandQueue = clCreateCommandQueue( clContext, clDeviceID, 0, clErr );

  var String source = "\n\
__kernel void square(\n\
   __global int* output,\n\
   const unsigned int width,\n\
   const unsigned int height,\n\
   const float center_re,\n\
   const float center_im,\n\
   const unsigned int zoom\n\
   )\n\
{\n\
  int i = get_global_id(0);\n\
\n\
  unsigned breadth;\n\
  if ( width < height )\n\
    breadth = width;\n\
  else\n\
    breadth = height;\n\
\n\
  float zoomAdjustment = pow( (float)1.1, (float)zoom );\n\
\n\
  unsigned y = i / width;\n\
  unsigned x = i % width;\n\
\n\
  float pos_re = 4.0 / zoomAdjustment * ((float)x - (width - breadth)/2) / breadth - 2.0/zoomAdjustment - center_re;\n\
  float pos_im = 4.0 / zoomAdjustment * ((float)y - (height - breadth)/2) / breadth - 2.0/zoomAdjustment - center_im;\n\
\n\
  float z_re = pos_re;\n\
  float z_im = pos_im;\n\
  int dwell;\n\
  for ( dwell = 0; dwell < 255; ++dwell )\n\
  {\n\
    if ( z_re * z_re + z_im * z_im >= 4.0 )\n\
      break;\n\
    float new_z_re = z_re*z_re - z_im*z_im + pos_re;\n\
    z_im = 2.0*z_re*z_im + pos_im;\n\
    z_re = new_z_re;\n\
  }\n\
  output[i] = dwell;\n\
}\n\
";
  var cl_program clProgram = clCreateProgramWithSource( clContext, source, clErr );
  clErr = clBuildProgram( clProgram, clDeviceIDs, "" );
  var cl_kernel clKernel = clCreateKernel( clProgram, "square", clErr );

  var Data NULL;

  var Integer[] results;
  results.resize( count );
  var cl_mem output = clCreateBuffer( clContext, CL_MEM_WRITE_ONLY, results.dataSize, NULL, clErr );

  var Scalar center_re = -0.341704;
  var Scalar center_im = 0.558186;
  var Size zoom = 72;
  clErr = clSetKernelArg( clKernel, 0, output.dataSize, output.data )
    | clSetKernelArg( clKernel, 1, width.dataSize, width.data )
    | clSetKernelArg( clKernel, 2, height.dataSize, height.data )
    | clSetKernelArg( clKernel, 3, center_re.dataSize, center_re.data )
    | clSetKernelArg( clKernel, 4, center_im.dataSize, center_im.data )
    | clSetKernelArg( clKernel, 5, zoom.dataSize, zoom.data );

  var Size local = width;
  var Size global = count;
  var cl_event[] eventWaitList;
  var cl_event event;
  clErr = clEnqueueNDRangeKernel( clCommandQueue, clKernel, 1, NULL, global.data, local.data, eventWaitList, event );
  clErr = clFinish( clCommandQueue );

  clErr = clEnqueueReadBuffer( clCommandQueue, output, CL_TRUE, 0, results.dataSize, results.data, eventWaitList, event );

  for ( var Size row=0; row<height; ++row )
  {
    var String rowString;
    for ( var Size col=0; col<width; ++col )
    {
      var Integer dwell = results[row*width + col];
      if ( dwell & 192 )
        rowString += "#";
      else if ( dwell & 48 )
        rowString += "*";
      else if ( dwell & 12 )
        rowString += ".";
      else
        rowString += " ";
    }
    report(rowString);
  }
}
