echo "Check CPU"
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
echo "Set CPU Performance"
for x in /sys/devices/system/cpu/*/cpufreq/; do echo 4100000 | sudo tee $x/scaling_max_freq; done
echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu5/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu6/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu7/cpufreq/scaling_governor
#cpufreq-set -d 3.00 GHz

echo "Re-check CPU"
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

