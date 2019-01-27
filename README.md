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
