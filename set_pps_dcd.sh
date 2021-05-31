#bash
# sudo apt-get install setserial ntp pps-tools
ntpd
service ntp restart
modprobe pps_ldisc
setserial /dev/ttyS0 low_latency
ldattach 18 /dev/ttyS0 #0
#ppstest /dev/pps0 #0
