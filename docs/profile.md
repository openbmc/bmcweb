LD_LIBRARY_PATH=/tmp LD_PRELOAD=libprofiler.so CPUPROFILE=/tmp/profile ./bmcweb

scp ed@hades.jf.intel.com:/home/ed/webserver/buildarm/bmcweb /tmp
scp ed@hades.jf.intel.com:/home/ed/gperftools/.libs/libprofiler.so /tmp