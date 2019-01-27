import time
import os
import math

import argparse

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--pulsar-name-iar",default="", type=str, required=True)
    parser.add_argument("-t", "--toggle-diode", action='store_true')
    parser.add_argument("-u", "--use-utc", action='store_true')
    return parser.parse_args()

def main():
    args = parse_args()
    print (args.pulsar_name_iar+".iar")
    print args.toggle_diode
    file = open((args.pulsar_name_iar+".iar"),"r")
    for i in range(0,9):
	    file.readline()
    aux=file.readline()
    real_time=float(aux.split(",")[1])
    pulsar_name=str(args.pulsar_name_iar.split(".")[0])
    print ("Pulsar name "+str(pulsar_name))
    print ("Total time "+str(real_time))
    if (args.use_utc==True):
	utc = "-u"
    else:
	utc = ""
    start=time.time()
    exec_time=0.0
#ionice -c 1 /home/observacion/softw/uhd/host/build/examples/puma_release_1 "$@" -a "serial=312D587"
    while (exec_time<real_time):
	os.system("kill -9 $(pidof rx_samples_c)")
	if(args.toggle_diode==True):
		print "Noise CAL"
		#312D587
		#312D58D
		#nuevos A = 314F813 connected
		#nuevos B = 314C049 connected
		#Nuevos C = 312D5A3
		os.system("ionice -c 1 /home/ggancio/uhd_2/uhd/host/build/examples/rx_samples_c -p "+pulsar_name+" -t"+" -i "+str(real_time)+" -a \"serial=314F813\"" + " " + utc)
	else:
		print "Direct obs"
		os.system("ionice -c 1 /home/ggancio/uhd_2/uhd/host/build/examples/rx_samples_c -p "+pulsar_name+" -i "+str(real_time)+" -a \"serial=314F813\"" + " " + utc)
	end=time.time()
	exec_time=(end-start)/60
	real_time=math.ceil(real_time-((end-start)/60))
	if (real_time<1):
		real_time=1
	print exec_time
	print ("Total time "+str(real_time))
if __name__ == "__main__":
    main()



