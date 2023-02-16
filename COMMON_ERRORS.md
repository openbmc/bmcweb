# Commonly recurring errors in bmcweb

What follows is a list of common errors that new users to bmcweb tend to make
when operating within its bounds for the first time. If this is your first time
developing in bmcweb, the maintainers highly recommend reading and understanding
_all_ of common traps before continuing with any development. Every single one
of the examples below compile without warnings, but are incorrect in
not-always-obvious ways, or impose a pattern that tends to cause hard to find
bugs, or bugs that appear later. Every one has been submitted to code review
multiple times.

## 1. Directly dereferencing a pointer without checking for validity first

```cpp
int myBadMethod(const nlohmann::json& j){
    const int* myPtr = j.get_if<int>();
    return *myPtr;
}
```

This pointer is not guaranteed to be filled, and could be a null dereference.

## 2. String views aren't null terminated

```cpp
int getIntFromString(std::string_view s){
    return std::atoi(s.data());
}
```

This will give the right answer much of the time, but has the possibility to
fail when `string_view` is not null terminated. Use `from_chars` instead, which
takes both a pointer and a length

## 3. Not handling input errors

```cpp
int getIntFromString(const std::string& s){
    return std::atoi(s.c_str());
}
```

In the case where the string is not representable as an int, this will trigger
undefined behavior at system level. Code needs to check for validity of the
string, ideally with something like `from_chars`, and return the appropriate
error code.

## 4. Walking off the end of a string

```cpp
std::string getFilenameFromPath(const std::string& path){
    size_t index = path.find("/");
    if (index != std::string::npos){
        // If the string ends with "/", this will walk off the end of the string.
        return path.substr(pos + 1);
    }
    return "";
}
```

## 5. Using methods that throw (or not handling bad inputs)

```cpp
int myBadMethod(nlohmann::json& j){
    return j.get<int>();
}
```

This method throws, and bad inputs will not be handled

Commonly used methods that fall into this pattern:

- std::variant::get
- std::vector::at
- std::map::at
- std::set::at
- std::\<generic container type\>::at
- nlohmann::json::operator!=
- nlohmann::json::operator+=
- nlohmann::json::at
- nlohmann::json::get
- nlohmann::json::get_ref
- nlohmann::json::get_to
- nlohmann::json::operator\<\<
- nlohmann::json::operator\>\>
- std::filesystem::create_directory
- std::filesystem::rename
- std::filesystem::file_size
- std::stoi
- std::stol
- std::stoll

### Special note: JSON

`nlohmann::json::parse` by default
[throws](https://json.nlohmann.me/api/basic_json/parse/) on failure, but also
accepts an optional argument that causes it to not throw: set the 3rd argument
to `false`.

`nlohmann::json::dump` by default
[throws](https://json.nlohmann.me/api/basic_json/dump/) on failure, but also
accepts an optional argument that causes it to not throw: set the 4th argument
to `replace`. Although `ignore` preserves content 1:1, `replace` is preferred
from a security point of view.

### Special note: Boost

there is a whole class of boost asio functions that provide both a method that
throws on failure, and a method that accepts and returns an error code. This is
not a complete list, but users should verify in the boost docs when calling into
asio methods, and prefer the one that returns an error code instead of throwing.

- boost::asio::ip::tcp::acceptor::bind();
- boost::asio::ip::tcp::acceptor::cancel();
- boost::asio::ip::tcp::acceptor::close();
- boost::asio::ip::tcp::acceptor::listen();
- boost::asio::ip::address::make_address();

## 6. Blocking functions

bmcweb uses a single reactor for all operations. Blocking that reactor for any
amount of time causes all other operations to stop. The common blocking
functions that tend to be called incorrectly are:

- sleep()
- boost::asio::ip::tcp::socket::read()
- boost::asio::ip::tcp::socket::read_some()
- boost::asio::ip::tcp::socket::write()
- boost::asio::ip::tcp::socket::write_some()
- boost::asio::ip::tcp::socket::connect()
- boost::asio::ip::tcp::socket::send()
- boost::asio::ip::tcp::socket::wait()
- boost::asio::steady_timer::wait()

Note: an exception is made for filesystem/disk IO read and write. This is mostly
due to not having great abstractions for it that mate well with the async
system, the fact that most filesystem accesses are into tmpfs (and therefore
should be "fast" most of the time) and in general how little the filesystem is
used in practice.

## 7. Lack of locking between subsequent calls

While global data structures are discouraged, they are sometimes required to
store temporary state for operations that require it. Given the single threaded
nature of bmcweb, they are not required to be explicitly threadsafe, but they
must be always left in a valid state, and checked for other uses before
occupying.

```cpp
std::optional<std::string> currentOperation;
void firstCallbackInFlow(){
    currentOperation = "Foo";
}
void secondCallbackInFlow(){
    currentOperation.reset();
}
```

In the above case, the first callback needs a check to ensure that
currentOperation is not already being used.

## 8. Wildcard reference captures in lambdas

```cpp
std::string x; auto mylambda = [&](){
    x = "foo";
}
do_async_read(mylambda)
```

Numerous times, lifetime issues of const references have been injected into
async bmcweb code. While capturing by reference can be useful, given how
difficult these types of bugs are to triage, bmcweb explicitly requires that all
code captures variables by name explicitly, and calls out each variable being
captured by value or by reference. The above prototypes would change to
`[&x]()...` Which makes clear that x is captured, and its lifetime needs
tracked.

## 9. URLs should end in "/"

```cpp
BMCWEB("/foo/bar");
```

Unless you explicitly have a reason not to (as there is one known exception
where the behavior must differ) all URL handlers should end in "/". The bmcweb
route handler will detect routes ending in slash and generate routes for both
the route ending in slash and the one without. This allows both URLs to be used
by users. While many specifications do not require this, it resolves a whole
class of bug that we've seen in the past.

## 10. URLs constructed in aggregate

```cpp
std::string routeStart = "/redfish/v1";

BMCWEB_ROUTE(routestart + "/SessionService/")
```

Very commonly, bmcweb maintainers and contributors alike have to do audits of
all routes that are available, to verify things like security and documentation
accuracy. While these processes are largely manual, they can mostly be conducted
by a simple grep statement to search for urls in question. Doing the above makes
the route handlers no longer greppable, and complicates bmcweb patchsets as a
whole.

## 11. Not responding to 404

```cpp
BMCWEB_ROUTE("/myendpoint/<str>",
    [](Request& req, Response& res, const std::string& id){
     crow::connections::systemBus->async_method_call(
          [asyncResp](const boost::system::error_code ec,
                      const std::string& myProperty) {
              if (ec)
              {
                  messages::internalError(asyncResp->res);
                  return;
              }
              ... handle code
          },
          "xyz.openbmc_project.Logging",
          "/xyz/openbmc_project/mypath/" + id,
          "xyz.MyInterface", "GetAll", "");
});
```

All bmcweb routes should handle 404 (not found) properly, and return it where
appropriate. 500 internal error is not a substitute for this, and should be only
used if there isn't a more appropriate error code that can be returned. This is
important, because a number of vulnerability scanners attempt injection attacks
in the form of `/myendpoint/foobar`, or `/myendpoint/#$*(%)&#%$)(\*&` in an
attempt to circumvent security. If the server returns 500 to any of these
requests, the security scanner logs it as an error for followup. While in
general these errors are benign, and not actually a real security threat, having
a clean security run allows maintainers to minimize the amount of time spent
triaging issues reported from these scanning tools.

An implementation of the above that handles 404 would look like:

```cpp
BMCWEB_ROUTE("/myendpoint/<str>",
    [](Request& req, Response& res, const std::string& id){
     crow::connections::systemBus->async_method_call(
          [asyncResp](const boost::system::error_code ec,
                      const std::string& myProperty) {
              if (ec == <error code that gets returned by not found>){
                  messages::resourceNotFound(res);
                  return;
              }
              if (ec)
              {
                  messages::internalError(asyncResp->res);
                  return;
              }
              ... handle code
          },
          "xyz.openbmc_project.Logging",
          "/xyz/openbmc_project/mypath/" + id,
          "xyz.MyInterface", "GetAll", "");
});
```

Note: A more general form of this rule is that no handler should ever return 500
on a working system, and any cases where 500 is found, can immediately be
assumed to be
[a bug in either the system, or bmcweb.](https://github.com/openbmc/bmcweb/blob/master/DEVELOPING.md#error-handling)

## 12. Imprecise matching

```cpp
void isInventoryPath(const std::string& path){
    if (path.find("inventory")){
        return true;
    }
    return false;
}
```

When matching dbus paths, HTTP fields, interface names, care should be taken to
avoid doing direct string containment matching. Doing so can lead to errors
where fan1 and fan11 both report to the same object, and cause behavior breaks
in subtle ways.

When using dbus paths, rely on the methods on `sdbusplus::message::object_path`.
When parsing HTTP field and lists, use the RFC7230 implementations from
boost::beast.

Other commonly misused methods are: boost::iequals. Unless the standard you're
implementing (as is the case in some HTTP fields) requires case insensitive
comparisons, casing should be obeyed, especially when relying on user-driven
data.

- boost::starts_with
- boost::ends_with
- std::string::starts_with
- std::string::ends_with
- std::string::rfind

The above methods tend to be misused to accept user data and parse various
fields from it. In practice, there tends to be better, purpose built methods for
removing just the field you need.

## 13. Complete replacement of the response object

```cpp
void getMembers(crow::Response& res){
  res.jsonValue = {{"Value", 2}};
}
```

In many cases, bmcweb is doing multiple async actions in parallel, and
orthogonal keys within the Response object might be filled in from another task.
Completely replacing the json object can lead to convoluted situations where the
output of the response is dependent on the _order_ of the asynchronous actions
completing, which cannot be guaranteed, and has many time caused bugs.

```cpp
void getMembers(crow::Response& res){
  res.jsonValue["Value"] = 2;
}
```

As an added benefit, this code is also more efficient, as it avoids the
intermediate object construction and the move, and as a result, produces smaller
binaries.

Note, another form of this error involves calling nlohmann::json::reset(), to
clear an object that's already been filled in. This has the potential to clear
correct data that was already filled in from other sources.
