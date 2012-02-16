F = require('Fabric').createClient();

op = F.DG.createOperator("videoSeekTime");
op.setSourceCode(
'use FabricVIDEO;\n'+
'operator videoSeekTime(\n'+
'  io VideoHandle handle,\n'+
'  io Scalar time,\n'+
'  io RGB pixels[],\n'+
'  io RGB cache[][Integer]\n'+
'  ) {\n'+
'  Integer frameIndex = floor(time * handle.fps + 0.5);\n'+
'  if(cache.has(frameIndex)) {\n'+
'    pixels = cache[frameIndex];\n'+
'    return;\n'+
'  }\n'+
'\n'+
'  FabricVIDEOSeekTime(handle, time);\n'+
'  FabricVIDEOGetAllPixels(handle, pixels);\n'+
'\n'+
'  cache[frameIndex] = pixels;\n'+
'}\n'
);
op.setEntryFunctionName("videoSeekTime");
if (op.getErrors().length > 0) {
  console.log(op.getErrors());
  if (op.getDiagnostics().length > 0) {
    console.log(op.getDiagnostics());
  }
}

F.close();

