# OpenBMC Webserver Development

1. ### Performance targets
    As OpenBMC is intended to be deployed on an embedded system, care should be
    taken to avoid expensive constructs, and memory usage.  In general, our
    performance and metric targets are:

    - Binaries and static files should take up < 1MB of filesystem size
    - Memory usage should remain below 10MB at all times
    - Application startup time should be less than 1 second on target hardware
      (AST2500)

2. ### Asynchronous programming
   Care should be taken to ensure that all code is written to be asynchronous in
   nature, to avoid blocking methods from stopping the processing of other
   tasks.  At this time the webserver uses boost::asio for it async framework.
   Threads should be avoided if possible, and instead use async tasks within
   boost::asio.

3. ### Secure coding guidelines
   Secure coding practices should be followed in all places in the webserver

    In general, this means:
    - All buffer boundaries must be checked before indexing or using values
    - All pointers and iterators must be checked for null before dereferencing
    - All input from outside the application is considered untrusted, and should
      be escaped, authorized and filtered accordingly.  This includes files in
      the filesystem.
    - All error statuses are checked and accounted for in control flow.
    - Where applicable, noexcept methods should be preferred to methods that use
      exceptions
    - Explicitly bounded types should be preferred over implicitly bounded types
      (like std::array<int, size> as opposed to int[size])
    - no use of [Banned
      functions](https://github.com/intel/safestringlib/wiki/SDL-List-of-Banned-Functions
      "Banned function list")

4. ### Error handling
   Error handling should be constructed in such a way that all possible errors
   return valid HTTP responses.  The following HTTP codes will be used commonly
    - 200 OK - Request was properly handled
    - 201 Created - Resource was created
    - 401 Unauthorized - Request didn't posses the necessary authentication
    - 403 Forbidden - Request was authenticated, but did not have the necessary
      permissions to accomplish the requested task
    - 404 Not found - The url was not found
    - 500 Internal error - Something has broken within the OpenBMC web server,
      and should be filed as a bug

    Where possible, 307 and 308 redirects should be avoided, as they introduce
    the possibility for subtle security bugs.

5. ### Startup times
   Given that the most common target of OpenBMC is an ARM11 processor, care
   needs to be taken to ensure startup times are low.  In general this means:

    - Minimizing the number of files read from disk at startup.  Unless a
      feature is explicitly intended to be runtime configurable, its logic
      should be "baked in" to the application at compile time.  For cases where
      the implementation is configurable at runtime, the default values should
      be included in application code to minimize the use of nonvolatile
      storage.
    - Avoid excessive memory usage and mallocs at startup.

6. ### Compiler features
    - At this point in time, the webserver sets a number of security flags in
      compile time options to prevent misuse.  The specific flags and what
      optimization levels they are enabled at are documented in the
      CMakeLists.txt file.
    - Exceptions are currently enabled for webserver builds, but their use is
      discouraged.  Long term, the intent is to disable exceptions, so any use
      of them for explicit control flow will likely be rejected in code review.
      Any use of exceptions should be cases where the program can be reasonably
      expected to crash if the exception occurs, as this will be the future
      behavior once exceptions are disabled.
    - Run time type information is disabled
    - Link time optimization is enabled

7. ### Authentication
   The webserver shall provide the following authentication mechanisms.
    - Basic authentication
    - Cookie authentication
    - Token authentication

    There shall be connection between the authentication mechanism used and
    resources that are available over it. The webserver shall employ an
    authentication scheme that is in line with the rest of OpenBMC, and allows
    users and privileges to be provisioned from other interfaces.

8. ### Web security
   The OpenBMC webserver shall follow the latest OWASP recommendations for
   authentication, session management, and security.

9. ### Performance
   The performance priorities for the OpenBMC webserver are (in order):
    1. Code is readable and clear
    2. Code follows secure guidelines
    3. Code is performant, and does not unnecessarily abstract concepts at the
       expense of performance
    4. Code does not employ constructs which require continuous system
       resources, unless required to meet performance targets.  (example:
       caching sensor values which are expected to change regularly)

10. ### Abstraction/interfacing
   In general, the OpenBMC webserver is built using the data driven design.
   Abstraction and Interface guarantees should be used when multiple
   implementations exist, but for implementations where only a single
   implementation exists, prefer to make the code correct and clean rather than
   implement a concrete interface.

11. ### phosphor webui
   The webserver should be capable of hosting phosphor-webui, and implementing
   the required flows to host the application.  In general, all access methods
   should be available to the webui.

12. ### Redfish
   bmcweb's Redfish implementation, including Redfish OEM Resources, shall
   conform to the Redfish specification. Please keep bmcweb's [Redfish support
   document](https://github.com/openbmc/bmcweb/blob/master/Redfish.md) updated.
   OEM schemas should conform and be developed in line with the rules in
   [OEM SCHEMAS](https://github.com/openbmc/bmcweb/blob/master/OEM_SCHEMAS.md).

13. ### Common errors
   A number of examples of common errors are captured in the common errors doc.
   It is recommended that developers read and understand all of them before
   starting any openbmc development.
   [Common Errors](https://github.com/openbmc/bmcweb/blob/master/COMMON_ERRORS.md).


## clang-tidy

clang-tidy is a tool that can be used to identify coding style violations, bad
design patterns, and bug prone constructs.  The checks are implemented in the
.clang-tidy file in the root of bmcweb, and are expected to be passing.  To
run, the best way is to run the checks in yocto.

```
# check out meta-clang in your openbmc root
cd openbmc
git clone https://github.com/kraj/meta-clang

# add the meta-clang layer to BBLAYERS in $BBPATH/conf/bblayers.conf
<path_to_your_build_dir>/meta-clang

# Add this line to $BBPATH/conf/local.conf to build bmcweb with clang
TOOLCHAIN_pn-bmcweb = "clang"

# and build
bitbake bmcweb

# Open devshell (this will open a shell)
bitbake -c devshell bmcweb

# cd into the work dir
cd oe-workdir/bmcweb-1.0+git999
# run clang tidy
clang-tidy --header-filter=".*" -p . $BBPATH/workspace/sources/bmcweb/src/webserver_main.cpp
```
