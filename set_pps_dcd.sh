#bash
# sudo apt-get install setserial ntp pps-tools
#server tick.iar.unlp.edu.ar prefer minpoll 4 maxpoll 4
#server ntp.aggo-conicet.gob.ar
#server 127.127.22.0                   # ATOM(PPS)
#fudge 127.127.22.0 flag3 1            # enable PPS API
#server ntp-time.for.mydomain
#server otherntp.server.org
#server ntp.research.gov



ntpd
service ntp restart
modprobe pps_ldisc
setserial /dev/ttyS0 low_latency
ldattach 18 /dev/ttyS0 #0
#ppstest /dev/pps0 #0
