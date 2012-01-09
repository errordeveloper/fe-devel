FABRIC = require('Fabric').createClient();

loadOp = FABRIC.DependencyGraph.createOperator("load");
loadOp.setEntryFunctionName("load");
loadOp.setSourceCode('\
use FabricOBJ;\n\
\n\
operator load(\n\
  io String url,\n\
  io FabricResource resource,\n\
  io Data objParseHandle\n\
  )\n\
{\n\
  report "Loaded " + url + " (mime type " + resource.mimeType + ")";\n\
  report "OBJ data size is " + resource.data.dataSize();\n\
  FabricOBJDecode(resource.data.data(), resource.data.dataSize(), objParseHandle);\n\
}\n\
');
if (loadOp.getDiagnostics().length > 0 ) {
  console.log(JSON.stringify(loadOp.getDiagnostics()));
}

loadOpBinding = FABRIC.DG.createBinding();
loadOpBinding.setOperator(loadOp);
loadOpBinding.setParameterLayout([
  "self.url",
  "self.resource",
  "self.objParseHandle"
]);

rlnode = FABRIC.DependencyGraph.createResourceLoadNode("rlnode");
rlnode.addMember("objParseHandle", "Data");
rlnode.bindings.append(loadOpBinding);
rlnode.setData("url", 0, "file:test.obj");

resizeOp = FABRIC.DependencyGraph.createOperator("resize");
resizeOp.setEntryFunctionName("resize");
resizeOp.setSourceCode('\
use FabricOBJ;\n\
operator resize(\n\
  io Data objParseHandle,\n\
  io Size newSize\n\
  )\n\
{\n\
  FabricOBJGetNbPoints(objParseHandle, newSize);\n\
  report "rlnode: resized to " + newSize + " points";\n\
}\n\
');
if (resizeOp.getDiagnostics().length > 0 ) {
  console.log(JSON.stringify(resizeOp.getDiagnostics()));
}

resizeBinding = FABRIC.DG.createBinding();
resizeBinding.setOperator( resizeOp );
resizeBinding.setParameterLayout([
  "rlnode.objParseHandle",
  "self.newCount"
]);

setDataOp = FABRIC.DependencyGraph.createOperator("setData");
setDataOp.setEntryFunctionName("setData");
setDataOp.setSourceCode('\
use FabricOBJ;\n\
operator setData(\n\
  io Data objParseHandle,\n\
  io Vec3 positions<>\n\
  )\n\
{\n\
  FabricOBJGetPointsSliced(objParseHandle, positions);\n\
  report "rlnode: setData to " + positions.size + " points";\n\
  FabricOBJFreeParsedData(objParseHandle);\n\
}\n\
');
if (setDataOp.getDiagnostics().length > 0 ) {
  console.log(JSON.stringify(setDataOp.getDiagnostics()));
}

setDataBinding = FABRIC.DG.createBinding();
setDataBinding.setOperator( setDataOp );
setDataBinding.setParameterLayout([
  "rlnode.objParseHandle",
  "self.position<>"
]);

node = FABRIC.DG.createNode("dataNode");
node.addMember("position", "Vec3");
node.bindings.append(resizeBinding);
node.bindings.append(setDataBinding);
node.setDependency(rlnode, "rlnode");
if ( node.getErrors().length > 0 )
  console.log(JSON.stringify((node.getErrors())));
node.evaluate();

FABRIC.flush();
