# pulsar_obs
Acquisition code to be used with Ettus B2xx boards for pulsar observation. 

Based from the example rx_samples_c.c from the ettus examples.
Its not a prety code, but it works.
the py code is part of the 'run' scripts.

The code is used as part of the software developments from the PUMA Project, for Pulsar Monitoring in Argentina, using the two 30 meter Radiotelescopes from the Argentine Intitute of Radioastronomy.

http://puma.iar.unlp.edu.ar/


Note, to use presto tools with our location and codes, we need to edit the following files from...
TEMPO:
obsys.dat

PRESTO: 
telescope_to_tempocode() in misc_utils.c
make_polycos() in polycos.c

SIGPROC-style filterbank format data,
get_telescope_name() in sigproc_fb.c

and aliases.c

This data will be used by the .fil header file, from the .iar file

Telescope ID,

IAR-A1: 19

IAR-A2: 23

DSA-3: 24

CLTC: 25


Machine ID,

USRP: 21


#Now build tempo

cd $PULSARSOFTWARE_DIR/tempo

./prepare

./configure F77=gfortran --prefix=$PULSARSOFTWARE_DIR LIBS=-lgslcblas CFLAGS=-f$

make

make install


#Now build PRESTO

cd $PRESTO/src

make makewisdom

make prep

make

#For some reason it does not find slalib in the first run

make

make clean

cd $PRESTO/python

make

make clean


