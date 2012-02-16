#!/usr/bin/python
import sys
import os
import exceptions
import glob
import platform

# construct the filepath based on this python script's path
basePath = __file__.replace('\\','/').rpartition('/')[0]
files = []

verbose = False

# RUN THE PARSER
def main():

  jsonsourcePath = False
  cpptemplatePath = False
  cppsourcePath = False
  klsourcePath = False

  # parse the command line options
  for i in range(1,len(sys.argv)):
    arg = sys.argv[i]
    if not arg.startswith('--') or arg.find('=') == -1:
      message = "ERROR: Command line arguments need to start with a double --\nValid arguments are:\n--template=path/to/template,\n--header=path/to/single/header (can be used multiple times)\n--jsonresult=path/to/json (file will be generated)\n--cppresult=path/to/cppfile (ogl.cpp, will be generated)"
      print(message)
      raise(Exception(message))
    arg = arg[2:100000]
    arg = arg.partition('=')
    if arg[0].lower() == "header":
      files.append(arg[2])
    elif arg[0].lower() == "template":
      cpptemplatePath = arg[2]
    elif arg[0].lower() == "jsonresult":
      jsonsourcePath = arg[2]
    elif arg[0].lower() == "cppresult":
      cppsourcePath = arg[2]
    elif arg[0].lower() == "klresult":
      klsourcePath = arg[2]
      
  if len(files) == 0 or not cpptemplatePath or not jsonsourcePath or not cppsourcePath or not klsourcePath:
      message = "Some command line options are missing\nUsage:\ngenerate.py --header=path/to/single/header --template=path/to/cpptemplate --cppresult=path/to/generated/cpp --klresult=path/to/generated/kl --jsonresult=path/to/generated/json\nYou can use --header multiple times to parse multiple headers.\nOptionally you can specify --jsresult=path/to/javascript to generate a JavaSCript file as well.\n"
      print(message)
      raise(Exception(message))
  
  # LET'S FIND THE CONSTANTS AND FUNCTIONS BASED ON KEYWORDS
  constants = []
  functions = []
  exports = {}
  for file in files:
    lines = open(file).read().split('\n')
    i = 0
    while(i < len(lines)):
      line = lines[i]
      line = line.replace('#if defined','#ifdef')
      
      while(line.strip().endswith(',')):
        i += 1
        line += lines[i]
      if line.startswith("GLAPI "):
        functions.append(line)
      elif(line.startswith("#define GL_") or
           line.startswith("#define GLX_") or
           line.startswith("#define WGL_")):
        constants.append(line)
      elif line.find('GET_VAR') > 15:
        constants.append(line)
      elif not line.startswith("#define") and line.find('_EXPORT')  > -1:
        export = line.strip(';').split(' ');
        if(len(export) == 3):
          exports[export[2]] = export[1]
      elif line.startswith('extern') and line.find('"C"') == -1:
        if line.find('(') > -1:
          functions.append(line)
        else:
          export = line.strip(';').split(' ');
          if(len(export) == 3):
            if not exports.has_key(export[2]):
              exports[export[2]] = export[1]
      elif line.startswith('GLAPI'):
        functions.append(line)
      elif line.find('GET_FUN') > 15:
        functions.append(line)
      elif line.startswith('typedef') and line.find('(') > -1:
        type = line.split(' ')[1]
        name = line.partition(')')[0].partition('(')[2].strip().rpartition(' ')[2]
        if not exports.has_key(name):
          value = '(' + line.rpartition('(')[2].partition(')')[0]+')'
          exports[name] = [type,value]
      i += 1
      
  # define suffixes to skip
  suffixes = [
    'MESA',
    'SUN',
    'SUNX',
    'NV',
    'ATI',
    'GREMEDY',
    'IBM',
    'SGIX',
    'AMD',
    'APPLE',
    'SGIS',
    'WIN',
    'INTEL',
    'OES',
    'SGI',
    'HP',
    'INGR'
  ]
  
  # LOOP OVER ALL DEFINED FUNCTIONS, AND REGISTER THEM
  registeredConstants = {}
  for i in range(len(constants)):
    # REPLACE MACROS WITH THEIR DEFINED FUNCTIONS
    if constants[i].find('(') > -1:
      name = constants[i].split(' ')[1]
      macro = constants[i].partition('(')[2].partition(')')[0]
      if(not exports.has_key(macro)):
        raise(Exception("Macro "+macro+" has not been found."))
      while(exports.has_key(macro)):
        macro = exports[macro]
      constants[i] = '#define '+name+' '+macro

    split = constants[i].strip(';').split(' ')
    if(len(split) == 3):
      name = split[1]
      hardwareSpecific = False
      for j in range(len(suffixes)):
        if name.find('_'+suffixes[j]) > -1:
          hardwareSpecific = True
          break
      if hardwareSpecific:
        continue
      value = split[2]
      if not hardwareSpecific and not registeredConstants.has_key(name) and not value == '0xFFFFFFFFFFFFFFFF':
        registeredConstants[name] = value;
        
  # CREATE THE SOURCE CODE FOR EACH CONSTANT
  jsonConstants = []
  sceneGraphConstants = []
  for name in registeredConstants:
    value = registeredConstants[name]
    if value.startswith('0x'):
      jsonConstants.append('const Size '+name+' = '+value+';')
      sceneGraphConstants.append('\\"'+name+'\\":'+str(int(value,0)))
    elif value.isdigit():
      jsonConstants.append('const Size '+name+' = '+value+';')
      sceneGraphConstants.append('\\"'+name+'\\":'+value)
    
  # DEFINE A MAPPING FOR THE DATATYPE
  # oglTYPE: [C++type, trace format, trace cast, KL Type]
  knownCTypes = {
    'void':['void','void*','','Data'],
    'bool':['bool','%d','int','Boolean'],
    'int':['int','%d','int','Integer'],
    'char':['char','%s','const char*','String'],
    'int64_t':['int64_t','%d','int','Integer'],
    'uint':['unsigned','0x%04X','unsigned','Integer'],
    'GLfloat':['GLfloat','%f','float','Scalar'],
    'GLint':['GLint','%d','int','Integer'],
    'GLsizei':['GLsizei','%d','int','Integer'],
    'GLushort':['unsigned','0x%04X','unsigned','Integer'],
    'GLvoid':['GLvoid','void*','','Data'],
    'GLboolean':['GLboolean','%d','int','Boolean'],
    'GLint':['GLint','%d','int','Integer'],
    'GLuint':['GLuint','0x%04X','unsigned','Integer'],
    'GLenum':['GLenum','%d','int','Integer'],
    'GLdouble':['GLdouble','%f','float','Scalar'],
    'GLbyte':['GLbyte','%d','int','Byte'],
    'GLubyte':['GLubyte','0x%04X','unsigned','Integer'],
    'GLhalf':['GLhalf','%d','integer','Integer'],
    'GLshort':['GLshort','%d','int','Integer'],
    'GLchar':['GLchar','%s','const char*','String'],
    'GLintptr':['GLintptr','%d','int','Integer'],
    'GLsizeiptr':['GLsizeiptr','%d','int','Integer'],
    'GLclampf':['GLclampf','%f','float','Scalar'],
    'GLclampd':['GLclampd','%f','float','Scalar'],
    'GLbitfield':['GLbitfield','0x%04X','unsigned','Integer'],
  }
  
  klKeyWords = {
    'in':'inValue',
    'io':'ioValue',
    'var':'varValue',
  }
  
  # LOOP ALL FUNCTIONS AND DEFINE THEIR CODE
  functionsCode = []
  klFunctionsCode = []
  knownFunctions = {}
  
  klFunctionsCode.append('function fglSetDebuggingEnabled( Boolean enable )= "fglSetDebuggingEnabled";')
  klFunctionsCode.append('function glGetVersion(io String version) = "glGetVersion_wrapper";')
  klFunctionsCode.append('function glewIsSupported(io String token, io Boolean supported) = "glewIsSupported_wrapper";')
  
  for i in range(len(functions)):

    # FIRST CHECK IF THE FUNCTION USES A MACRO
    if functions[i].find("GET_FUN") > -1:
      name = functions[i].partition('(')[2].partition(')')[0]
      macro = name
      while(exports.has_key(str(macro))):
        macro = exports[str(macro)]
      if not(len(macro) == 2):
        continue
      functions[i] = macro[0]+" "+name.strip('_')+" "+macro[1]+';'
      
    # APPLY SOME FORMATTING
    function = functions[i]
    if function.startswith('extern '):
      function = function[7:1000]
    if function.startswith('GLAPI '):
      function = function[6:1000]
    function = function.replace('BOOL','bool')
    function = function.replace('*','* ').replace(' *','*').replace('*  ','* ')
    function = function.replace(', ',',').replace('( ','(').replace(' )',')').replace(' (','(')
    function = function.replace('GLAPIENTRY ','')
    function = function.replace('APIENTRY ','')

    # EXTRACT THE FUNCTION'S NAME
    name = function.partition(' ')[2].partition('(')[0].rpartition(' ')[2]
    
    # IF WE HAVE A GLEW NAME, REPLACE IT WITH GL
    if name.startswith('glew'):
      name = 'gl'+name[4:1000]
    if knownFunctions.has_key(name):
      continue
    if not name.startswith('gl'):
      continue
    
    # CHECK IF THIS IS A HARDWARE SPECIFIC FUNCTION AND SKIP THOSE
    hardwareSpecific = False
    for i in range(len(suffixes)):
      if name.endswith(suffixes[i]):
        hardwareSpecific = True
        break
    if hardwareSpecific:
      continue
          
    # LET'S FIGURE OUT THE RETURN TYPE
    returnType = function.partition('(')[0].rpartition(' ')[0]
    returnTypeKey = returnType.rpartition(' ')[2].strip('*')
    if not knownCTypes.has_key(returnTypeKey):
      if verbose:
        print("Warning: Skipped function '"+name+"' due to unknown return type '"+returnType+ "'.")
      continue

    # DEFINE KL FUNCTIONS
    klParameters = []
    klCast = []
    cParameters = []
    
    # EXTRACT ALL OF THE VARIABLES (BETWEEN BRACES)
    variables = function.partition('(')[2].rpartition(')')[0].split(',')
    if len(variables) == 1 and variables[0] == 'void':
      variables = []
      
    # LOOP OVER ALL VARIABLES
    allTypesValid = True
    traceFormat = []        # THE PRINTF FORMAT TOKENS (USED FOR TRACE)
    traceVars = []          # THE NAME OF THE VARIABLES TO PASS TO PRINTF (TRACE)
    varNamesHead = []       # THE NAMES OF THE VARIABLES FOR THE FUNCTION DEFINITION
    additionalCodePre = []  # CODE TO RUN PRE THE GL CALL
    additionalCodePost = [] # CODE TO RUN POST THE GL CALL
    for i in range(len(variables)):
      
      # STRIP OFF WHITESPACES
      variables[i] = variables[i].strip()
      
      # DEFINE A NAME FOR THE VARIABLE IF THERE IS NONE
      if variables[i].find(' ') == -1 or variables[i].endswith('*'):
        variables[i] = variables[i] + ' var'+str(i)
        
      # EXTRACT THE TYPE FOR EACH VARIABLE
      fulltype = variables[i].rpartition(' ')[0].strip()
      
      # GET THE SIMPLE TYPE, AKA char OUT OF const char*
      type = fulltype.strip('*').strip()
      if type.startswith('const '):
        type = type[6:1000]
        
      # ENSURE THE VARIABLE TYPE IS KNOWN
      if not knownCTypes.has_key(type) or variables[i].find('[') > -1:
        allTypesValid = False
        if verbose:
          print("Warning: Skipped function '"+name+"' due to unknown parameter type '"+variables[i]+ "'.")
        break

      # deal with special case functions
      if name.lower().find('string') > -1 and len(variables) == 1 and variables[i].count('*') == 1:
        # this means we have a string!
        traceFormat.append(fulltype)
        klParameters.append('io String '+klvarname+'')
        cParameters.append('KL::String &'+varname)
        klCast.append('('+fulltype+')'+varname+'.data()')
        continue

      # EXTRACT THE NAME OF THE VARIABLE
      varname = variables[i].rpartition(' ')[2]
      klvarname = varname
      if klKeyWords.has_key(klvarname):
        klvarname = klKeyWords[klvarname]
        
      # DEAL WITH STRINGS OR DUAL POINTERS
      if variables[i].count('*') == 2 or type == 'char' or type == 'GLchar':
        if type == 'char' or type == 'GLchar':

          # DEAL WITH A STIRNG ARRAY
          if(variables[i].find('const') > -1):
            
            # IF WE HAVE A CONST CHAR, WE WANT TO JUST READ IT
            if fulltype.find('**') > -1:
              additionalCodePre.append('  KL::VariableArray<KL::Data> '+varname+'Data;')
              additionalCodePre.append('  '+varname+'Data.resize('+varname+'.size());')
              additionalCodePre.append('  for(KL::Size '+varname+'Counter = 0;'+varname+'Counter<'+varname+'.size();'+varname+'Counter++)')
              additionalCodePre.append('    '+varname+'Data['+varname+'Counter] = (KL::Data)'+varname+'['+varname+'Counter].data();')
              klParameters.append('io String '+klvarname+'[]')
              cParameters.append('const KL::VariableArray<KL::String> &'+varname)
              klCast.append('('+fulltype+')&'+varname+'Data[0]')
              traceFormat.append('char *')
            else:
              klParameters.append('io String '+klvarname)
              cParameters.append('const KL::String &'+varname)
              klCast.append('('+fulltype+')'+varname+'.data()')
              traceFormat.append('\'%s\'')
              traceVars.append( varname+'.data()' )
          else:
            
            # IF WE DON'T HAVE A CONST CHAR, WE NEED TO WRITE TO IT
            additionalCodePre.append('  char * '+varname+'Str = new char[1024];')
            additionalCodePost.append('  '+varname+' = KL::String((const char*)'+varname+'Str);')
            additionalCodePost.append('  delete( '+varname+'Str );')
            klParameters.append('io String '+klvarname)
            cParameters.append('KL::String & '+varname)
            klCast.append('('+fulltype+')'+varname+'Str')
            traceFormat.append('char *')
        else:
          # BAIL OUT, NO DUAL POINTERS ARE ALLOWED. SKIP THIS FUNCTION
          if verbose:
            print("Warning: Skipped function '"+name+"' due to invalid double pointr parameter '"+variables[i]+ "'.")
          allTypesValid = False
          break
      else:
        # IF WE HAVE A TRACE FORMAT TYPE, AND WE ARE NOT A POINTER, LET'S TRACE US
        if len(knownCTypes[type][2]) > 0 and variables[i].find('*') == -1:
          traceFormat.append(knownCTypes[type][1])
          traceVars.append('('+knownCTypes[type][2]+')'+varname)
        else:
          traceFormat.append(type+'*')
          
        # IF WE HAVE A POINTER (BUT NOT 'void*', AS IT IS A VALUE BY ITSELF)
        if variables[i].find('*') > -1 and variables[i].find('void') == -1:

          digit = ''
          for k in range(len(name)-1,0,-1):
            if name[k:k+1].isdigit():
              digit = name[k:k+1]
              break
          if len(digit) > 0 and name.lower().find('matrix') > -1:
            # let's convert to matrix
            klParameters.append('io Mat'+digit+digit+' '+klvarname+'[]')
            cParameters.append('KL::VariableArray<KL::Mat'+digit+digit+'> & '+varname)
            klCast.append('('+fulltype+')&'+varname+'[0]')
          else:
            if knownCTypes[type][3] == 'Data':
              if(variables[i].count('*') == 2):
                if variables[i].startswith('const'):
                  klParameters.append('in Data '+klvarname+'[]')
                else:
                  klParameters.append('io Data '+klvarname+'[]')
                cParameters.append('KL::Data '+varname+'[]')
                klCast.append('('+fulltype+')&'+varname+'[0]')
              else:
                if variables[i].startswith('const'):
                  klParameters.append('in Data '+klvarname)
                else:
                  klParameters.append('io Data '+klvarname)
                cParameters.append('KL::Data '+varname)
                klCast.append('('+fulltype+')'+varname)
            elif variables[i].startswith('const'):
              # THIS CAN BE DONE BASED ON THE DIGITS
              klParameters.append('io '+knownCTypes[type][3]+' '+klvarname+'['+digit+']')
              cParameters.append('const KL::VariableArray<KL::'+knownCTypes[type][3]+'> & '+varname)
              klCast.append('('+fulltype+')&'+varname+'[0]')
            else:
              # this is really flaky, it should be >Data, but that's not possible.
              # the rvalue is still able to write to though.
              klParameters.append('io '+knownCTypes[type][3]+' '+klvarname+'[]')
              cParameters.append('KL::VariableArray<KL::'+knownCTypes[type][3]+'> & '+varname)
              klCast.append('('+fulltype+')&'+varname+'[0]')
        else:
          klParameters.append('in '+knownCTypes[type][3]+' '+klvarname)
          cParameters.append('KL::'+knownCTypes[type][3]+' '+varname)
          klCast.append('('+fulltype+')'+varname)
      varNamesHead.append(varname)
    if not allTypesValid:
      continue
    
    # REMEMBER THE FUNCTION
    knownFunctions[name] = function
    
    klReturnType = returnType
    if klReturnType.find('void') == -1 or klReturnType.count('*') > 0:
      # special case, string returning functions
      if name.lower().find('string') > -1 and len(variables) == 1 and returnType.count('*') == 1:
        returnTypeKey = "char"
        klReturnType = 'KL::String'
      elif klReturnType.find('void') > -1 and klReturnType.count('*') > 0:
        klReturnType = 'KL::Data'
      else:
        klReturnType = 'KL::'+knownCTypes[returnTypeKey][3]
      
    # IF WE HAVE VARIABLES
    if len(cParameters) > 0:
      if returnType.find('void') > -1 or returnType.count('*') > 0:
        functionsCode.append('FABRIC_EXT_EXPORT '+klReturnType+' '+name+'_wrapper(')
      else:
        functionsCode.append('FABRIC_EXT_EXPORT '+klReturnType+' '+name+'_wrapper(')
      for i in range(len(cParameters)-1):
        functionsCode.append('  '+cParameters[i]+',')
      functionsCode.append('  '+cParameters[len(cParameters)-1])
      functionsCode.append('){')
    else:
      if returnType.find('void') > -1 or returnType.count('*') > 0:
        functionsCode.append('FABRIC_EXT_EXPORT '+returnType+' '+name+'_wrapper()')
      else:
        functionsCode.append('FABRIC_EXT_EXPORT KL::'+knownCTypes[returnTypeKey][3]+' '+name+'_wrapper()')
      functionsCode.append('{')
    functionsCode.append('  if(!'+name+')')
    functionsCode.append('    throwException( "OGL: '+name+': unsupported on this hardware" );')
    if not name == 'glGetError' and not name == 'gluErrorString':
      functionsCode.append('  _clearError();')
    if name.lower() == 'glbegin':
      functionsCode.append('  _incBracket();')

    functionsCode.append('  if ( fglDebuggingEnabled ) {')
    if len(traceVars) > 0:
      functionsCode.append('    printf("'+name+'( '+str(', ').join(traceFormat)+' );\\n", '+str(', ').join(traceVars)+');')
    else:
      functionsCode.append('    printf("'+name+'( '+str(', ').join(traceFormat)+' );\\n");')
    functionsCode.append('  }')

    functionsCode.extend(additionalCodePre)
    prefix = ''
    if returnType.find('void') == -1 or returnType.count('*') > 0:
      prefix = returnType+' result = '
    if len(klCast) > 0:
      functionsCode.append('  '+prefix+name+'( '+str(', ').join(klCast)+' );')
    else:
      functionsCode.append('  '+prefix+name+'();')

    if name.lower() == 'glend':
      functionsCode.append('  _decBracket();')
    if not name == 'glGetError' and not name == 'gluErrorString':
      functionsCode.append('  _checkError("'+name+'");')
    functionsCode.extend(additionalCodePost)

    # IF WE HAVE A RETURN TYPE
    if returnType.find('void') == -1 or returnType.count('*') > 0:
      if returnType.count('*') == 0:
        functionsCode.append('  if ( fglDebuggingEnabled ) {')
        functionsCode.append('    printf("  -> returned %u\\n", (unsigned)result );')
        functionsCode.append('  }')
      if klReturnType == "KL::String":
        functionsCode.append('  return ('+klReturnType+')(const char*)result;')
      else:
        functionsCode.append('  return ('+klReturnType+')result;')

    functionsCode.append('}\n')
    klFunction = 'function ';
    if returnTypeKey.find('void') == -1 or returnType.count('*') > 0:
      klFunction = klFunction + knownCTypes[returnTypeKey][3]+' '
    klFunction = klFunction + name+'( '
    klFunction = klFunction + ', '.join(klParameters) + ' ) = '+"'"+name+'_wrapper'+"'"+';'
    klFunctionsCode.append(klFunction)
  
  # define the core json types
  jsonTypes = [];
  jsonTypes.append(str("""
struct Vec2 {
  Scalar x;
  Scalar y;
};
""").strip())
  jsonTypes.append(str("""
struct Vec3 {
  Scalar x;
  Scalar y;
  Scalar z;
};
""").strip())
  jsonTypes.append(str("""
struct Vec4 {
  Scalar x;
  Scalar y;
  Scalar z;
  Scalar t;
};
""").strip())
  jsonTypes.append(str("""
struct Mat22 {
  Vec2 row0;
  Vec2 row1;
};
""").strip())
  jsonTypes.append(str("""
struct Mat33 {
  Vec3 row0;
  Vec3 row1;
  Vec3 row2;
};
""").strip())
  jsonTypes.append(str("""
struct Mat44 {
  Vec4 row0;
  Vec4 row1;
  Vec4 row2;
  Vec4 row3;
};
""").strip())

  # JSON structure for SceneGraph constants
  jsConstants = "{"+str(",").join(sceneGraphConstants)+"}"

  open(jsonsourcePath,'w').write('{\n  "libs": "FabricOGL",\n  "code": "FabricOGL.kl",\n  "jsConstants":"' + jsConstants + '"\n}\n')
  print("Generated JSON result: "+jsonsourcePath)
  #open('/development/temp/test.kl','w').write(str('\n').join(jsonConstants)+'\n\n'+str('\n').join(klFunctionsCode)+'\n\nfunction entry()\n{\n  report("valid");\n}\n')
  
  open(klsourcePath,'w').write(str("\n").join(jsonTypes)+"\n"+str("\n").join(jsonConstants)+"\n"+str("\n").join(klFunctionsCode))
  print("Generated KL result: "+klsourcePath)
  #open('/development/temp/test.kl','w').write(str('\n').join(jsonConstants)+'\n\n'+str('\n').join(klFunctionsCode)+'\n\nfunction entry()\n{\n  report("valid");\n}\n')
  
  # LOAD THE TEMPLATE AND FILL IT
  template = open(cpptemplatePath).read();
  template = template.replace("####FUNCTIONS####",str('\n').join(functionsCode))
  # WRITE OUT TO OUR SOURCE FILE
  open(cppsourcePath,"w").write(template)
  print("Generated CPP result: "+cppsourcePath)

if __name__ == '__main__':
  main()
