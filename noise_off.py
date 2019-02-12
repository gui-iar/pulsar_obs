import parallel
p = parallel.Parallel()  # open LPT1 or /dev/parport0
p.setData(0x00)
