# OpenBMC webserver #

This component attempts to be a "do everything" embedded webserver for openbmc.


## Capabilities ##
At this time, the webserver implements a few interfaces:
+ Authentication middleware that supports cookie and token based authentication, as well as CSRF prevention backed by linux PAM authentication credentials.
+ An (incomplete) attempt at replicating phosphor-dbus-rest interfaces in C++.  Right now, a few of the endpoint definitions work as expected, but there is still a lot of work to be done.  The portions of the interface that are functional are designed to work correctly for phosphor-webui, but may not yet be complete.
+ Replication of the rest-dbus backend interfaces to allow bmc debug to logged in users.
+ An initial attempt at a read-only redfish interface.  Currently the redfish interface targets ServiceRoot, SessionService, AccountService, Roles, and ManagersService.  Some functionality here has been shimmed to make development possible.  For example, there exists only a single user role.
+ SSL key generation at runtime.  If an RSA key and cert pair are not available to the server at runtime, keys are generated using the openssl routines, and written to disk.
+ Static file hosting.  Currently, static files are hosted from the fixed location at /usr/share/www.  This is intended to allow loose coupling with yocto projects, and allow overriding static files at build time.
+ Dbus-monitor over websocket.  A generic endpoint that allows UIs to open a websocket and register for notification of events to avoid polling in single page applications.  (this interface may be modified in the future due to security concerns.


## Crow patches ##
The crow project has had a number of additions to make it more useful for use in the OpenBmc Project.  A non-exhaustive list is below.  At the time of this writing, the crow project is not accepting patches, so for the time being crow will simply be checked in as is.

+ Applied clang-format to the whole crow tree.  This was done without regard for arrays and fixed data structures, but was deemed to be overall better than the inconsistent formatting that existed in upstream previously.
+ Crow server now calls stop before destruction of the Crow app object.
+ Fixed a bug where timed out websockets would seg fault the system by accessing a destroyed socket object without null checks when in SSL mode.
+ Added a TestSocketAdapter class that could be used to unit test server behavior without utilizing a socket.
+ Added the "get_routes" call to both the app and the routing Trie class that allows consumers to poll the server for all (or a subset of) registered web routes in the system.
+ Hardcoded the websocket implementation for binary mode, instead of leaving protocol unspecified.
+ Move most uses of std::unordered_map to boost::flat_map to lower memory consumption, and (in some cases) to improve memory locality.
+ Adjust the add_headers mechanism to use a fixed string instead of a full map implementation to avoid unnecessary mallocs and reduce the number of scatter gather buffers on an http response.
+ Change server name header from Crow/0.1 to iBMC
+ Starts the http server io_service inside the main thread, instead of creating a new thread.
+ Removes all CROW_MSVC_WORKAROUND flags.
+ Removes the behavior that causes a 301 redirect for paths that end in "/", and simply returns the endpoint requested.  This was done for redfish compatibility.
+ Removes the built in crow/json.hpp package and adds nlohmann json package as the first class json package for crow.
+ Move uses of boost::array to std::array where possible.
+ Add the ability to get a reference to the crow::request object on websocket connection to allow checking header values.
+ Patch http handler to call middlewares on websocket connections to allow authentication to be applied appropriately.
+ Adds an is_secure flag to provide information about whether or not the payload was delivered over ssl.

