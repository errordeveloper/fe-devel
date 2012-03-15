import fabric
import sys

fabricClient = fabric.createClient()

# Take the number of terms to compute on the command
# line default to 10

numTerms = 10
if len( sys.argv ) > 1:
  numTerms = int( sys.argv[1] )

# Load the source code and compile into an executable

compilation = fabricClient.KLC.createCompilation()
compilation.addSource('computeTerm.kl', open('computeTerm.kl').read())
compilation.addSource('subTerms.kl', open('sumTerms.kl').read())
executable = compilation.run()

# Summing a series is a simple map-reduce operation where
# we generate the terms of the series
print(
  fabricClient.MR.createReduce(
    fabricClient.MR.createArrayGenerator(
      fabricClient.MR.createConstValue('Size', numTerms),
      executable.resolveArrayGeneratorOperator('computeTerm')
      ),
    executable.resolveReduceOperator('sumTerms')
    ).produce()
  )

# Close the Fabric Engine client.  If the client isn't closed
# then the Python module will keep this script alive!

fabricClient.close()
