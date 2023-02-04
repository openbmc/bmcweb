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
- nlohmann::json::items
- nlohmann::json::operator\<\<
- nlohmann::json::operator\>\>
- nlohmann::json::begin
- nlohmann::json::operator[]
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

The nlohmann::json class represents ANY json object, but unforuntately includes
a number of helper functions that allow it to "act" like it only represents an
json dict. In the majority of uses, these will not cause a problem, but if a
type that isn't an object (0.0, "foo", or null) is already present in the
object, these methods will
[throw an exception](https://json.nlohmann.me/api/basic_json/operator%5B%5D/#exceptions),
which generally goes uncaught, and can lead to potential for denial of service
on bad input. Care should be taken when operating with a nlohmann::json object,
and should generally prefer to use an nlohmann::json::object_t or
nlohmann::json::array_t where appropriate. There is quite a large amount of
code in bmcweb that does not handle the distinction between nlohmann::json and
nlohmann::json::object_t correctly. These will be corrected over time.

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

Note, unit tests can now find this for redfish routes.

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
     dbus::utility::async_method_call(
          [asyncResp](const boost::system::error_code& ec,
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
     dbus::utility::async_method_call(
          [asyncResp](const boost::system::error_code& ec,
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

## 14. Very long lambda callbacks

```cpp
dbus::utility::getSubTree("/", interfaces,
                         [asyncResp](boost::system::error_code& ec,
                                     MapperGetSubTreeResult& res){
                            <many lines of code>
                         })
```

Inline lambdas, while useful in some contexts, are difficult to read, and have
inconsistent formatting with tools like clang-format, which causes significant
problems in review, and in applying patchsets that might have minor conflicts.
In addition, because they are declared in a function scope, they are difficult
to unit test, and produce log messages that are difficult to read given their
unnamed nature.

Prefer to either use std::bind_front, and a normal method to handle the return,
or a lambda that is less than 10 lines of code to handle an error inline. In
cases where std::bind_front cannot be used, such as in
sdbusplus::asio::connection::async_method_call, keep the lambda length less than
10 lines, and call the appropriate function for handling non-trivial transforms.

```cpp
void afterGetSubTree(std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     boost::system::error_code& ec,
                     MapperGetSubTreeResult& res){
   <many lines of code>
}

dbus::utility::getSubTree("/xyz/openbmc_project/inventory", interfaces,
                         std::bind_front(afterGetSubTree, asyncResp));
```

See also the
[Cpp Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f11-use-an-unnamed-lambda-if-you-need-a-simple-function-object-in-one-place-only)
for generalized guidelines on when lambdas are appropriate. The above
recommendation is aligned with the C++ Core Guidelines.

## 15. Using async_method_call where there are existing helper methods

```cpp
dbus::utility::async_method_call(
    respHandler, "xyz.openbmc_project.ObjectMapper",
    "/xyz/openbmc_project/object_mapper",
    "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
    "/xyz/openbmc_project/inventory", 0, interfaces);
```

It's required to use D-Bus utility functions provided in the file
include/dbus_utility.hpp instead of invoking them directly. Using the existing
util functions will help to reduce the compilation time, increase code reuse and
uniformity in error handling. Below are the list of existing D-Bus utility
functions

- getProperty
- getAllProperties
- checkDbusPathExists
- getSubTree
- getSubTreePaths
- getAssociatedSubTree
- getAssociatedSubTreePaths
- getAssociatedSubTreeById
- getAssociatedSubTreePathsById
- getDbusObject
- getAssociationEndPoints
- getManagedObjects

## 16. Using strings for DMTF schema Enums

```cpp
sensorJson["ReadingType"] = "Frequency";

```

Redfish Schema Enums and types are auto generated using
scripts/generate_schema_enums.py. The generated header files contain the redfish
enumerations which must be used for JSON response population.

```cpp
#include "generated/enums/sensor.hpp"
sensorJson["ReadingType"] = sensor::ReadingType::Frequency;
```

## 17. Duplicated map lookups

```cpp
std::unordered_map<std::string, std::string> mymap;

if (mymap.contains("mykey")){
    std::string& myvalue = mymap["mykey"];
}
```

While functionally correct doing the above results in more code generated, and
duplicates the key lookup in the map. The above code also makes it more
difficult to use const for "mymap", as operator[] is not const. Sometimes at()
is used in place of operator[] to get around this, but also duplicates the
lookup.

As a minor consequence, the lookup key "mykey" is now duplicated, or needs to be
loaded into a scope variable.

```cpp
std::unordered_map<std::string, std::string> mymap;

auto it = mymap.find("mykey");
if (it != mymap.end()){
    std::string& myvalue = it->second;
}
```

## 18. Auto lvalue for nontrivial function returns

```cpp
auto x = my_function();
```

Given this line of code, what is the type of x? Does x need to be checked for
null before using? Does x hold a value that is making a copy?

Explicitly declare the type when the function is nontrivial.

```cpp
int x = my_function();
```

Note, that exceptions to this rule are present when using templated data
structures holding a member (map/vector/set). Given that the type is known, and
generic data structures are well understood by both reviewers and authors,
explicitly naming the iterator impacts both readability and can impact
correctness in some const cases. Given that, consider the following code.

```cpp
std::map<int, int> mymap;

auto it = mymap.find(0);
```

Given that the returned type is still present in the code, and the majority of
C++ data structures follow a pattern that can be traced from the snippet above,
auto is an improvement in readability here.
