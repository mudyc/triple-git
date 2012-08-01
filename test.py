
import subprocess

# test add
open('autotest', 'w').close() 
for i in range(1, 101):
    if subprocess.call(["./triples", "autotest", "add", "sub", "pred", str(i)]) != 0:
        print "add error:", i;

    #f = fopen("autotest")
    #for i in range(1, i):
        
