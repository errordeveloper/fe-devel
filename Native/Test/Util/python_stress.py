import fabric
import psutil
import os
import time

def create_lots_nodes():
  p = psutil.Process( os.getpid() )

  num = 10000
  for i in range(0, 100):
    print "mem: " + str( round( p.get_memory_info().vms / 1024.0 / 1024.0, 2 ) ) + " MB"
    print "starting loop " + str(i) + "..."
    c = fabric.createClient()
    for i in range(0, num):
      c.DG.createNode("foo"+str(i))
    print "flushing..."
    c.flush()
    print "closing..."
    c.close()

def pass_lots_data():
  c = fabric.createClient()

  iters = 32
  size = 2048
  num = 0

  start = time.time()

  n = c.DG.createNode("test")
  n.addMember( "input", "String" )
  n.addMember( "output", "String" )
  n.resize( size )

  op = c.DG.createOperator( "testOp" )
  op.setEntryPoint( "entry" )
  op.setSourceCode( "operator entry( io String input, io String output ) { output = 'output: ' + input; }" )

  b = c.DG.createBinding()
  b.setOperator( op )
  b.setParameterLayout( [ "self.input", "self.output" ] )

  n.bindings.append( b )

  for i in range( 0, iters ):
    for i in range( 0, size ):
      n.setData( "input", i, "input"+str( num ))
      num += 1

    n.evaluate()

    for i in range( 0, size ):
      x = n.getData( "output", i )

  c.close()

  print 'execution time = ' + str( time.time() - start )

pass_lots_data()
