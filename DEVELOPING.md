# OpenBMC Webserver Development

## Guidelines

### Performance targets

As OpenBMC is intended to be deployed on an embedded system, care should be
taken to avoid expensive constructs, and memory usage. In general, our
performance and metric targets are:

- Binaries and static files should take up < 1MB of filesystem size
- Memory usage should remain below 10MB at all times
- Application startup time should be less than 1 second on target hardware
  (AST2500)

### Asynchronous programming

Care should be taken to ensure that all code is written to be asynchronous in
nature, to avoid blocking methods from stopping the processing of other tasks.
At this time the webserver uses boost::asio for it async framework. Threads
should be avoided if possible, and instead use async tasks within boost::asio.

### Secure coding guidelines

Secure coding practices should be followed in all places in the webserver

In general, this means:

- All buffer boundaries must be checked before indexing or using values
- All pointers and iterators must be checked for null before dereferencing
- All input from outside the application is considered untrusted, and should be
  escaped, authorized and filtered accordingly. This includes files in the
  filesystem.
- All error statuses are checked and accounted for in control flow.
- Where applicable, noexcept methods should be preferred to methods that use
  exceptions
- Explicitly bounded types should be preferred over implicitly bounded types
  (like std::array<int, size> as opposed to int[size])
- no use of
  [Banned functions](https://github.com/intel/safestringlib/wiki/SDL-List-of-Banned-Functions "Banned function list")

### Error handling

Error handling should be constructed in such a way that all possible errors
return valid HTTP responses. The following HTTP codes will be used commonly

- 200 OK - Request was properly handled
- 201 Created - Resource was created
- 401 Unauthorized - Request didn't posses the necessary authentication
- 403 Forbidden - Request was authenticated, but did not have the necessary
  permissions to accomplish the requested task
- 404 Not found - The url was not found
- 500 Internal error - Something has broken within the OpenBMC web server, and
  should be filed as a bug

Where possible, 307 and 308 redirects should be avoided, as they introduce the
possibility for subtle security bugs.

### Startup times

Given that the most common target of OpenBMC is an ARM11 processor, care needs
to be taken to ensure startup times are low. In general this means:

- Minimizing the number of files read from disk at startup. Unless a feature is
  explicitly intended to be runtime configurable, its logic should be "baked in"
  to the application at compile time. For cases where the implementation is
  configurable at runtime, the default values should be included in application
  code to minimize the use of nonvolatile storage.
- Avoid excessive memory usage and mallocs at startup.

### Compiler features

- At this point in time, the webserver sets a number of security flags in
  compile time options to prevent misuse. The specific flags and what
  optimization levels they are enabled at are documented in the CMakeLists.txt
  file.
- Exceptions are currently enabled for webserver builds, but their use is
  discouraged. Long term, the intent is to disable exceptions, so any use of
  them for explicit control flow will likely be rejected in code review. Any use
  of exceptions should be cases where the program can be reasonably expected to
  crash if the exception occurs, as this will be the future behavior once
  exceptions are disabled.
- Run time type information is disabled
- Link time optimization is enabled

### Authentication

The webserver shall provide the following authentication mechanisms.

- Basic authentication
- Cookie authentication
- Token authentication

There shall be connection between the authentication mechanism used and
resources that are available over it. The webserver shall employ an
authentication scheme that is in line with the rest of OpenBMC, and allows users
and privileges to be provisioned from other interfaces.

### Web security

The OpenBMC webserver shall follow the latest OWASP recommendations for
authentication, session management, and security.

### Performance

The performance priorities for the OpenBMC webserver are (in order):

1. Code is readable and clear
2. Code follows secure guidelines
3. Code is performant, and does not unnecessarily abstract concepts at the
   expense of performance
4. Code does not employ constructs which require continuous system resources,
   unless required to meet performance targets. (example: caching sensor values
   which are expected to change regularly)

### Abstraction/interfacing

In general, the OpenBMC webserver is built using the data driven design.
Abstraction and Interface guarantees should be used when multiple
implementations exist, but for implementations where only a single
implementation exists, prefer to make the code correct and clean rather than
implement a concrete interface.

### webui-vue

The webserver should be capable of hosting webui-vue, and implementing the
required flows to host the application. In general, all access methods should be
available to the webui.

### Redfish

bmcweb's Redfish implementation, including Redfish OEM Resources, shall conform
to the Redfish specification. Please keep bmcweb's
[Redfish support document](https://github.com/openbmc/bmcweb/blob/master/Redfish.md)
updated. OEM schemas should conform and be developed in line with the rules in
[OEM SCHEMAS](https://github.com/openbmc/bmcweb/blob/master/OEM_SCHEMAS.md).

### Common errors

A number of examples of common errors are captured in the common errors doc. It
is recommended that developers read and understand all of them before starting
any openbmc development.
[Common Errors](https://github.com/openbmc/bmcweb/blob/master/COMMON_ERRORS.md).

### Commit messages

Project commit message formatting should be obeyed
[link](https://github.com/openbmc/docs/blob/master/CONTRIBUTING.md#formatting-commit-messages)

Commit messages should answer the following questions:

- Why are the changes useful? Given that bmcweb is a user-facing daemon, commits
  adding new functionality should include statements about how the commit in
  question is useful to the user.

- What changes would a user expect to see? This includes new parameters, new
  resources, and new or changed properties. Any route changes should be
  explicitly called out.

- Are there compatibility concerns? Is this change backward compatible for
  clients? If not, what commit would be broken, and how old is it? Have clients
  been warned? (ideally on the mailing list) link the discussion.

Commit messages should be line wrapped 50/72.

### Compatibility

> Don't make your users mad -
> [Greg K-H](https://git.sr.ht/~gregkh/presentation-application_summit/tree/main/keep_users_happy.pdf)

The kernel has very similar rules around compatibility that we should aspire to
follow in the footsteps of.

To that end, bmcweb will do its' best to insulate clients from breaking api
changes. Being explicit about this ensures that clients can upgrade their
OpenBMC version without issue, and resolves a significant bottleneck in getting
security patches deployed to users. Any change that's visible to a user is
potentially a breaking change, but requiring _all_ visible changes to be
configurable would increase the software complexity, therefore bmcweb makes
exceptions for things which a client is reasonably expected to code against:

- New items added to a collection
- Changes in UID for hypermedia resources (In line with Redfish spec)
- New properties added to a resource
- New versions of a given schema

Special note: Code exists in bmcweb that is missing upstream backends to make it
function. Given that compatibility requires the ability to use and test the
feature in question, changes to these methods, including outright removal, does
not constitute a breaking change.

Security: There may be cases where maintainers make explicit breaking changes in
the best interest of security; In these rare cases, the maintainers and
contributors will endeavor to avoid breaking clients as much as is technically
possible, but as with all security, impact will need to be weighed against the
security impact of not making changes, and judgement calls will be made, with
options to allow providing the old behavior.

### clang-tidy

clang-tidy is a tool that can be used to identify coding style violations, bad
design patterns, and bug prone constructs. The checks are implemented in the
.clang-tidy file in the root of bmcweb, and are expected to be passing.
[openbmc-build-scripts](https://github.com/openbmc/openbmc-build-scripts/blob/master/run-unit-test-docker.sh)
implements clang-tidy checks and is the recommended way to run these checks

### Logging Levels

Five bmcweb logging levels are supported, from least to most severity:

- debug
- info
- warning
- error
- critical

And their use cases:

- critical: Something went badly wrong, and we're no longer able to serve
  traffic. "critical" should be used when bmcweb encountered an event or entered
  a state that caused crucial function to stop working or when bmcweb
  encountered a fatal error.
- error: Something went wrong, and we weren't able to give the expected
  response. Service is still operational. "error" should be used for unexpected
  conditions that prevented bmcweb from fulfilling the request. "error" shall be
  used for 5xx errors.
- warning: A condition occurred that is outside the expected flows, but isn't
  necessarily an error in the webserver, or might only be an error in certain
  scenarios. For example, connection drops or 4xx errors.
- info: Information for the golden path debugging.
- debug: Information that's overly verbose such that it shouldn't be printed in
  all debug scenarios, but might be useful in some debug contexts.

### Enabling logging

bmcweb by default is compiled with runtime logging disabled, as a performance
consideration. To enable it in a standalone build, add the logging level

```ascii
-Dlogging='<log-level>'
```

option to your configure flags. If building within Yocto, add the following to
your local.conf.

```bash
EXTRA_OEMESON:pn-bmcweb:append = "-Dbmcweb-logging='<log-level>'"
```

The choices of `<log-level>` can be

- `disabled`: Turns off all bmcweb log traces.

- `enabled`: Treated as `debug`

- The other option can be selected as described above
  [Logging Levels](DEVELOPING.md)
