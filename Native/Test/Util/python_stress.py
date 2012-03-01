import fabric
import psutil
import os

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

