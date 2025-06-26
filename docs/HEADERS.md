# bmcweb headers

**Why does bmcweb use so many headers? My build times are slow!**

TL;DR, History

bmcweb at one point was a crow-based project. Evidence of this can still be seen
in the http/.hpp files that still contain references to the crow namespaces.
Crow makes heavy use of headers and template meta programming, and doesn't ship
any cpp or implementation files, choosing to put everything in include once
headers. As bmcweb evolved, it needed more capabilities, so the core was ported
to Boost Beast, and what remains has very little similarity to crow anymore.
Boost::beast at the time we ported took the same opinion, relying on header
files and almost no implementation compile units. A large amount of the compile
time is taken up in boost::beast template instantiations, specifically for
boost::beast::http::message (ie Request and Response).

The initial solution that gets proposed is to just move everything as it exists
to separate compile units, making no other changes. This has been proposed and
implemented 3-4 times in the project, the latest of which is below. The intent
of this document is largely to save effort for the next person, so they can at
least start from the existing prior attempts.

<https://gerrit.openbmc.org/c/openbmc/bmcweb/+/49039>

Moving to cpp files without handling any architecture has the net result of
making total compilation slower, not faster, as the slowest-to-compile parts end
up getting compiled multiple times, then the duplicates deleted at link time.
This isn't great for the end result.

To actually effect the result that we'd like to see from multiple compile units,
there have been proposed a few ideas might provide some relief;

- Moving the Request and Response containers to opaque structures, so a majority
  of code only needs to #include the interface, not any of the template code.
  <https://gerrit.openbmc.org/c/openbmc/bmcweb/+/37445> Doing this exposed a
  number of mediocre practices in the route handlers, where routes made copies
  of requests/responses, relied on APIs that should've been internal, and other
  practices that make this migration less straightforward, but is still being
  pursued by maintainers over time.
- Moving the internals of Request/Response/Connection to rely on something like
  [http::proto](https://github.com/CPPAlliance/http_proto) which, written by the
  same author as boost::beast, claims to have significant reduction in compile
  time templates, and might not require abstracting the Request/Response
  objects.
- Reduce the bmcweb binary size to the point where link time optimization is not
  required for most usages. About half of the bmcweb build time is spent doing
  link time optimization, which, as of this time is required to keep bmcweb code
  small enough to deploy on an actual BMCs (See DEVELOPING.md for details). One
  could theoretically determine the source of where LTO decreases the binary
  size the most, and ensure that those were all in the same compile unit, such
  that they got optimized without requiring LTO.
