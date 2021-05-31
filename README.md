# pulsar_obs
Acquisition code to be used with Ettus B2xx boards for pulsar observation. 

Based from the example rx_samples_c.c from the ettus examples.
Its not a prety code, but it works.
the py code is part of the 'run' scripts.

The code is used as part of the software developments from the PUMA Project, for Pulsar Monitoring in Argentina, using the two 30 meter Radiotelescopes from the Argentine Intitute of Radioastronomy.
Instal and use of pulsar rx_samples

https://files.ettus.com/manual/page_build_guide.html

sudo apt-get install libboost-all-dev libusb-1.0-0-dev doxygen python3-docutils python3-mako python3-numpy python3-requests python3-ruamel.yaml python3-setuptools cmake build-essential

In case of error....  sudo apt-get install python-mako --reinstall


git clone https://github.com/EttusResearch/uhd.git

git checkout  UHD-3.10.2 or get the tar.gz

cd <uhd-repo-path>/host

  mkdir build

  cd build

  cmake ../
  
  make

  make test # This step is optional

  sudo make install
  
  sudo ldconfig
  
  
the branch shluld be...    UHD_003.010.002.HEAD-0-gbd6e21dc

install FFTW3 and FFTW3-dev  sudo apt-get install -y fftw3-dev 
  
Copy the git file rx_samples_c.c_20200x0x_Ax to host/examples, and replace the file rx_samples_c.c

edit xxx and add...
  
examples/CMakeFiles/rx_samples_c.dir/link.txt
  
  -lpthread -lfftw3

  examples/CMakeFiles/rx_samples_c.dir/flags.make
  
  -lfftw3
  
  build the examples.

At last implement the bash scripts, set_cpu_performance.sh and set_pps_dcd.sh
  
  http://puma.iar.unlp.edu.ar/


Note, to use presto tools with our location and codes, we need to edit the following files from...

TEMPO:

obsys.dat


PRESTO: 

telescope_to_tempocode() in misc_utils.c

make_polycos() in polycos.c

get_telescope_name() in sigproc_fb.c


SIGPROC: 

and aliases.c


This data will be used by the .fil header file, from the .iar file

Telescope ID,

IAR-A1: 19, "IAR1", "A1", "m"

IAR-A2: 20, "IAR2", "A2", "o",

DSA-3: 24, "DSA3", "D3", "p",

CLTC: 25, "CLTC", "CL", "q",


Machine ID,

RTL_Filterbank: 23
IAR_ROACH_v1: 24
IAR_SNAP_v1: 25


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


