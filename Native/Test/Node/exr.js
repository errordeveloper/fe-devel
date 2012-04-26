/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

op = FABRIC.DependencyGraph.createOperator("op");
op.setEntryPoint("load");
op.setSourceCode('\
use FabricEXR;\n\
\n\
struct Color\n\
{\n\
  Scalar r;\n\
  Scalar g;\n\
  Scalar b;\n\
  Scalar a;\n\
};\n\
\n\
operator load( io String url, io FabricResource resource )\n\
{\n\
  report("Loaded " + url + " (mime type " + resource.mimeType + ")");\n\
  report("EXR data size is " + resource.data.dataSize());\n\
  Size imageWidth, imageHeight;\n\
  Color imagePixels[];\n\
  FabricEXRDecode( resource.data.data(), resource.data.dataSize(), imageWidth, imageHeight, imagePixels );\n\
  report("Image dimensions are "+imageWidth+" by "+imageHeight);\n\
  report("Image pixels size is "+imagePixels.size);\n\
}\n\
');
if (op.getDiagnostics().length > 0)
  console.log(JSON.stringify(op.getDiagnostics()));

binding = FABRIC.DG.createBinding();
binding.setOperator(op);
binding.setParameterLayout([
  "loadnode.url",
  "loadnode.resource"
]);

rlnode = FABRIC.DependencyGraph.createResourceLoadNode("rlnode");
rlnode.setData("url", 0, "testfile://sample.exr");

node = FABRIC.DependencyGraph.createNode("node");
node.setDependency(rlnode, "loadnode");
node.bindings.append(binding);
node.evaluate();

FABRIC.close();
