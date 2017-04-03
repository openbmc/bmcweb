LD_LIBRARY_PATH=/tmp LD_PRELOAD=libprofiler.so CPUPROFILE=/tmp/profile ./bmcweb

scp ed@hades.jf.intel.com:/home/ed/webserver/buildarm/bmcweb /tmp -i /nv/.ssh/id_rsa
scp ed@hades.jf.intel.com:/home/ed/webserver/buildarm/getvideo /tmp -i /nv/.ssh/id_rsa
scp ed@hades.jf.intel.com:/home/ed/gperftools/.libs/libprofiler.so /tmp

echo 1 >> /sys/module/video_drv/parameters/debug
echo 8 > /proc/sys/kernel/printk