/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

var fs = require('fs');

var onCreateContextCallbacks = [];
var KLAppendMap = {};

FABRIC.define = function()
{
  if( arguments.length > 0 )
    arguments[arguments.length-1]();
};
FABRIC.UnitTests = {};
FABRIC.UnitTestUtils = {};
FABRIC.appendOnCreateContextCallback = function(callback) {
    onCreateContextCallbacks.push(callback);
  };
  
FABRIC.loadResourceURL = function(url) {
    url = url.replace('FABRIC_ROOT', '../../../Web');
    var data = fs.readFileSync(url, 'ascii');
    data = data.replace(/\0/g,'');//Not sure why I get /0 characters...
    var klClassName = url.match(/[^\/]+(?=\.kl)/gi);
    if( klClassName !== null ) {
      var klAppend = KLAppendMap[ klClassName[0] ];
      if( klAppend !== undefined ){
        //console.log(klAppend);
        data += klAppend;
      }
    }
    return data;
  }

function onPostLoad()
{
  for (var i = 0; i < onCreateContextCallbacks.length; ++i) {
    onCreateContextCallbacks[i](FABRIC);
  }
  onCreateContextCallbacks.length = 0;
}

FABRIC.UnitTestUtils.appendToKLCode = function(type, code) {
  if( KLAppendMap[type] === undefined )
    KLAppendMap[type] = code;
  else
    KLAppendMap[type] += '\n' + code;
}

function appendKLBinaryOpAdaptor(type, op, adaptor, otherType) {
  if( otherType == undefined )
    otherType = type;
  var code = "\nfunction " + type + ' ' + type + "." + adaptor + "( in " + otherType + " other ) {\n  return this " + op + " other;\n}\n\n";
  FABRIC.UnitTestUtils.appendToKLCode(type, code);
}

function appendKLInPlaceOpAdaptor(type, op, adaptor, otherType) {
  if( otherType == undefined )
    otherType = type;
  var code = "\nfunction " + type + "." + adaptor + "( in " + otherType + " other ) {\n this " + op + " other;\n}\n\n";
  FABRIC.UnitTestUtils.appendToKLCode(type, code);
}

FABRIC.UnitTestUtils.appendKLOpAdaptors = function(type, operators) {

  var code = "\nfunction " + type + ".set( in " + type + " other ) {\n this = other;\n}\n\n";
  FABRIC.UnitTestUtils.appendToKLCode(type, code);

  for( var i = 0; i < operators.length; ++i ) {
    var op = operators[i];
    var otherType = type;
    if( typeof op !== 'string' ) {
      var pair = op;
      op = pair[0];
      otherType = pair[1];
    }
    var postfix = '';
    if( type != otherType )
      postfix = otherType;
    switch (op) {
      case '+':
        appendKLBinaryOpAdaptor(type, '+', 'add'+postfix, otherType);
        break;
      case '+=':
        appendKLInPlaceOpAdaptor(type, '+=', 'add'+postfix+'InPlace', otherType);
        break;
      case '-':
        appendKLBinaryOpAdaptor(type, '-', 'subtract'+postfix, otherType);
        break;
      case '-=':
        appendKLInPlaceOpAdaptor(type, '-=', 'subtract'+postfix+'InPlace', otherType);
        break;
      case '*':
        appendKLBinaryOpAdaptor(type, '*', 'multiply'+postfix, otherType);
        break;
      case '*=':
        appendKLInPlaceOpAdaptor(type, '*=', 'multiply'+postfix+'InPlace', otherType);
        break;
      case '/':
        appendKLBinaryOpAdaptor(type, '/', 'divide'+postfix, otherType);
        break;
      case '/=':
        appendKLInPlaceOpAdaptor(type, '/=', 'divide'+postfix+'InPlace', otherType);
        break;
    }
  }
}

function defineInPlaceOpAdaptor(type, binaryop) {
  FABRIC.RT[type].prototype[binaryop+'InPlace'] = function(other) {
    var result = this[binaryop](other);
    this.set(result);
  };
}

FABRIC.UnitTestUtils.defineInPlaceOpAdaptors = function(type, operators) {
  for( var i = 0; i < operators.length; ++i ) {
    var op = operators[i];
    var otherType = type;
    if( typeof op !== 'string' ) {
      var pair = op;
      op = pair[0];
      otherType = pair[1];
    }
    var postfix = '';
    if( type != otherType )
      postfix = otherType;
    switch (op) {
      case '+=':
        defineInPlaceOpAdaptor(type, 'add'+postfix);
        break;
      case '-=':
        defineInPlaceOpAdaptor(type, 'subtract'+postfix);
        break;
      case '*=':
        defineInPlaceOpAdaptor(type, 'multiply'+postfix);
        break;
      case '/=':
        defineInPlaceOpAdaptor(type, 'divide'+postfix);
        break;
    }
  }
}

function roundNumber(num) {
	if(Math.abs(num) <= 10e-4)
		return 0;
	var prec4 = num.toPrecision(4);
	var roundedPrec4 = Math.round(prec4);
	if(prec4 == roundedPrec4)
		return roundedPrec4;
	else
		return prec4;
}

function getNumericalFields(type, path) {
  if( path === undefined )
    path = [];

  var result = [];
  for(var member in type)
  {
    path.push(member);
    var memberValue = type[member];
    if( typeof memberValue === 'object' ) {
      result = result.concat( getNumericalFields(memberValue, path) );
    }
    else if( typeof memberValue === 'number' ) {
      var pathCopy = [];
      for( var i = 0; i < path.length; ++i )
        pathCopy.push(path[i]);
      result.push(pathCopy);
    }
    path.pop();
  }
  return result;
}

function generateAppendResult(type)
{
  var fields = getNumericalFields(new FABRIC.RT[type]);

  //1) Generate JS function
  FABRIC.RT[type].prototype['appendResult'] = function(tests, results, testName) {
    tests.push(testName);
    var result = [];
    for( var i = 0; i < fields.length; i++ )
    {
      var field = this;
      for( var j = 0; j < fields[i].length; j++ )
      {
        var nextField = field[fields[i][j]];
        if(j == fields[i].length-1)
          result.push(nextField);
        else
          field = nextField;
      }
    }
    results.push(result);
  }
  FABRIC.RT[type].prototype['appendScalarResult'] = function(tests, results, testName, scalar) {
    tests.push(testName);
    results.push([scalar]);
  }
  FABRIC.RT[type].prototype['appendBooleanResult'] = function(tests, results, testName, boolean) {
    tests.push(testName);
    results.push([boolean?1.0:0.0]);
  }

  //2) Generate KL function
	//Note: we return dummy Scalar to tell KL that we don't modify the data (r-value)
  var klCode =  
       'function Scalar ' + type + '.appendScalarResult(io String tests[], io Scalar results[][], in String name, in Scalar value) {\n'
     + '  tests.push(name);\n'
     + '  Scalar data[];\n'
     + '  data.push(value);\n'
     + '  results.push(data);\nreturn 1.0;\n}\n\n'
     + 'function Scalar ' + type + '.appendBooleanResult(io String tests[], io Scalar results[][], in String name, in Boolean value) {\n'
     + '  this.appendScalarResult( tests, results, name, value?1.0:0.0 );\nreturn 1.0;\n}\n\n'
     + 'function Scalar ' + type + '.appendResult(io String tests[], io Scalar results[][], in String name) {\n'
     + '  tests.push(name);\n'
     + '  Scalar data[];\n';

  for( var i = 0; i < fields.length; i++ )
  {
    var path = '';
    for( var j = 0; j < fields[i].length; j++ )
    {
      path += '.' + fields[i][j];
      if(j == fields[i].length-1)
        klCode += '  data.push(Scalar(this'+path+'));\n';
    }
  }
  klCode += '  results.push(data);\nreturn 1.0;\n}\n\n';
  FABRIC.UnitTestUtils.appendToKLCode(type, klCode);
}

FABRIC.UnitTestUtils.loadType = function(type){
 require("../../../../Web/RT/" + type + ".js");
 generateAppendResult(type);
 onPostLoad();
}

FABRIC.UnitTestUtils.loadTestFile = function( prefix )
{
  var unitTestsCode = fs.readFileSync(prefix+'UnitTests.jskl', 'ascii');
  return unitTestsCode.replace(/\0/g,'');//Not sure why I get /0 characters...
}

FABRIC.UnitTestUtils.runTests = function(type, localVariables, unitTestsCode)
{
  if( unitTestsCode === undefined ) {
    unitTestsCode = FABRIC.UnitTestUtils.loadTestFile( type );
  }
  var testFuncName = 'run' + type + 'Tests';

  //1) Run in JS

  //Generate JS prefix / postfix code:
  var jsCodePrefix = 'FABRIC.UnitTests[\"'+testFuncName+'\"] = function(tests, results) {\n';
  if( localVariables !== undefined )
  {
    for( i = 0; i < localVariables.length; i++ )
    {
      var varType = localVariables[i][0];
      var varName = localVariables[i][1];
      if( varType === 'Scalar' )
        jsCodePrefix += 'var ' + varName + ' = 0.0;\n';
      else
        jsCodePrefix += 'var ' + varName + ' = new FABRIC.RT.'+ varType +';\n';
    }
  }
  var jsCode = jsCodePrefix + unitTestsCode + '\n}\n';

  eval(jsCode);

  var jsTests = [];
  var jsResults = [];
  FABRIC.UnitTests[testFuncName](jsTests, jsResults);

  //2) Run in KL

  //build code
  var klCodePrefix = 
      'use ' + type + ';\n\n'
    + 'operator '+testFuncName+'(io String tests[], io Scalar results[][]) {\n';
  var nbPrefixKLLines = 5;
  if( localVariables !== undefined )
  {
    nbPrefixKLLines += localVariables.length;
    for( i = 0; i < localVariables.length; i++ )
    {
      var varType = localVariables[i][0];
      var varName = localVariables[i][1];
      klCodePrefix += varType + ' ' + varName + ';\n';
    }
  }
  var klCode = klCodePrefix + unitTestsCode + '\n}\n';
  var klOp = FABRIC.DependencyGraph.createOperator(testFuncName);
  klOp.setEntryFunctionName(testFuncName);
  klOp.setSourceCode(klCode);

  var diagnostics = klOp.getDiagnostics();
  if (diagnostics.length > 0) {
    for( var i in diagnostics ) {
      var li = diagnostics[i].line;
      if(diagnostics[i].filename.match('.kl') === null) {
        li -= nbPrefixKLLines;
      }
      console.log(diagnostics[i].level +': '+ diagnostics[i].filename + ' (li ' + li + ', col ' + diagnostics[i].column + '): ' + diagnostics[i].desc);
    }
  }

  binding = FABRIC.DependencyGraph.createBinding();
  binding.setOperator(klOp);
  binding.setParameterLayout(["self.tests", "self.results"]);

  node = FABRIC.DependencyGraph.createNode( type + "TestNode" );
  node.addMember( "tests", "String[]" );
  node.addMember( "results", "Scalar[][]" );
  node.bindings.append(binding);

  var errors = node.getErrors();
  if ( errors.length > 0 ) {
    for ( var i in errors )
      console.log( errors[i] );
  }
  else {
    node.evaluate();
  }

  var klTests = node.getData( "tests" );
  var klResults = node.getData( "results" );

  //Compare KL and JS results
  if(jsTests.length != klTests.length || jsResults.length != klResults.length) {
    console.log('Unexpected: JS and KL have different number of tests');
  }

  for( var i in jsTests ) {
    var test = jsTests[i];
    if( test !== klTests[i] ) {
      console.log('Unexpected: JS and KL tests out of synch: JS test = '+ test + ', KL test = ' + klTests[i]);
    }

    var jsResult = jsResults[i];
    var klResult = klResults[i];
    if(jsResult.length != klResult.length) {
      console.log('Unexpected: test \''+ test + '\': JS and KL have different result sizes');
    }
    var resultString = '[';
    for( var j in jsResult ) {
      //Be careful with floating point precision: it's not the same in KL (32 bit) and JS (64 bit)
			var jsVal = roundNumber(jsResult[j]);
      var klVal = roundNumber(klResult[j]);
      if( jsVal !== klVal ) {
        console.log('Unexpected: test \''+ test + '\' value # ' + j + ' differs between JS ( ' + jsVal + ' ) and KL (' + klVal + ')');
      }
      if( resultString.length != 1 )
        resultString += ',';
      resultString += ' ' + jsVal;
    }
    resultString += ' ]';
    console.log( '\'' + test + '\': ' + resultString );
  }
};
