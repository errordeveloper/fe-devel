/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var ShaderSource = function( code, type ) {
	this.code = (code!=undefined) ? code : "";
	this.type = (type!=undefined) ? type : 0;
};
	
FABRIC.RegisteredTypesManager.registerType( 
	'ShaderSource', 
	{
		members: { 
			code:'String', type:'Integer' 
		},
		constructor: ShaderSource
	} 
);

var klCode = "";
klCode += "operator bar( io ShaderSource shaders[] ) {\n";
klCode += "\tfor( var Size i = 0; i < shaders.size(); i++ ) {\n";
klCode += "\t\treport( \"Type: \" + shaders[i].type );\n";
klCode += "\t\treport( \"Code: \" + shaders[i].code );\n";
klCode += "\t}\n";
klCode += "}\n";

console.log( klCode );

var sources = [];
sources.push( new ShaderSource( "Zero", 0 ) );
sources.push( new ShaderSource( "One", 1 ) );

var node = FABRIC.DependencyGraph.createNode("foo");
node.addMember( 'shaderSources', 'ShaderSource[]' );
node.setData( 'shaderSources', sources );

var op = FABRIC.DependencyGraph.createOperator("bar");
op.setEntryFunctionName("bar");
op.setSourceCode(klCode);

var binding = FABRIC.DependencyGraph.createBinding();
binding.setOperator( op );
binding.setParameterLayout( ['self.shaderSources' ] );

node.bindings.append( binding );
var errors = node.getErrors();
if ( errors.length > 0 ) {
  for ( var i in errors )
    console.log( errors[i] );
}
else {
  node.evaluate();
}

FABRIC.close();
