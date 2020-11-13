# Commonly recurring errors in bmcweb

What follows is a list of common errors that new users to bmcweb tend to make
when operating within its bounds for the first time.  If this is your first time
developing in bmcweb, the maintainers highly recommend reading and understanding
_all_ of common traps before continuing with any development.  Every single one
of the examples below compile without warnings, but are incorrect in
not-always-obvious ways, or impose a pattern that tends to cause hard to find
bugs, or bugs that appear later.  Every one has been submitted to code review
multiple times.

### Directly dereferencing a pointer without checking for validity first
```C++
int myBadMethod(const nlohmann::json& j){
    const int* myPtr = j.get_if<int>();
    return *myPtr;
}
```
This pointer is not guaranteed to be filled, and could be a null dereference.

### String views aren't null terminated
```C++
int getIntFromString(const std::string_view s){
    return std::atoi(s.data());
}
```
This will give the right answer much of the time, but has the possibility to
fail when string\_view is not null terminated.  Use from\_chars instead, which
takes both a pointer and a length

### Not handling input errors
```C++
int getIntFromString(const std::string& s){
    return std::atoi(s.c_str());
}
```
In the case where the string is not representable as an int, this will trigger
undefined behavior at system level.  Code needs to check for validity of the
string, ideally with something like from\_chars, and return the appropriate error
code.

### Walking off the end of a string
```C++
std::string getFilenameFromPath(const std::string& path){
    size_t index = path.find("/");
    if (index != std::string::npos){
        // If the string ends with "/", this will walk off the end of the string.
        return path.substr(pos + 1);
    }
    return "";
}
```

### Using methods that throw (or not handling bad inputs)
```C++
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
- nlohmann::json::get\_ref
- nlohmann::json::get\_to
- std::filesystem::create\_directory
- std::filesystem::rename
- std::filesystem::file\_size
- std::stoi
- std::stol
- std::stoll

#### special/strange case:

nlohmann::json::parse by default throws on failure, but also accepts a optional
argument that causes it to not throw.  Please consult the other examples in the
code for how to handle errors.


#### Special note: Boost
there is a whole class of boost asio functions that provide both a method that
throws on failure, and a method that accepts and returns an error code.  This is
not a complete list, but users should verify in the boost docs when calling into
asio methods, and prefer the one that returns an error code instead of throwing.

- boost::asio::ip::tcp::acceptor::bind();
- boost::asio::ip::tcp::acceptor::cancel();
- boost::asio::ip::tcp::acceptor::close();
- boost::asio::ip::tcp::acceptor::listen();
- boost::asio::ip::address::make\_address();

### Blocking functions

bmcweb uses a single reactor for all operations.  Blocking that reactor for any
amount of time causes all other operations to stop.  The common blocking
functions that tend to be called incorrectly are:

- sleep()
- boost::asio::ip::tcp::socket::read()
- boost::asio::ip::tcp::socket::read\_some()
- boost::asio::ip::tcp::socket::write()
- boost::asio::ip::tcp::socket::write\_some()
- boost::asio::ip::tcp::socket::connect()
- boost::asio::ip::tcp::socket::send()
- boost::asio::ip::tcp::socket::wait()
- boost::asio::steady\_timer::wait()

Note: an exception is made for filesystem/disk IO read and write.  This is
mostly due to not having great abstractions for it that mate well with the async
system, the fact that most filesystem accesses are into tmpfs (and therefore
should be "fast" most of the time) and in general how little the filesystem is
used in practice.

### Lack of locking between subsequent calls
While global data structures are discouraged, they are sometimes required to
store temporary state for operations that require it.  Given the single
threaded nature of bmcweb, they are not required to be explicitly threadsafe,
but they must be always left in a valid state, and checked for other uses
before occupying.

```C++
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

### Wildcard reference captures in lambdas
```
std::string x; auto mylambda = [&](){
    x = "foo";
}
do_async_read(mylambda)
```

Numerous times, lifetime issues of const references have been injected into
async bmcweb code.  While capturing by reference can be useful, given how
difficult these types of bugs are to triage, bmcweb explicitly requires that all
code captures variables by name explicitly, and calls out each variable being
captured by value or by reference.  The above prototypes would change to
[&x]()... Which makes clear that x is captured, and its lifetime needs tracked.


### URLs should end in "/"
```C++
BMCWEB("/foo/bar");
```
Unless you explicitly have a reason not to (as there is one known exception
where the behavior must differ) all URL handlers should end in "/".  The bmcweb
route handler will detect routes ending in slash and generate routes for both
the route ending in slash and the one without.  This allows both URLs to be
used by users.  While many specifications do not require this, it resolves a
whole class of bug that we've seen in the past.


### URLs constructed in aggregate
```C++
std::string routeStart = "/redfish/v1";

BMCWEB_ROUTE(routestart + "/SessionService/")
```
Very commonly, bmcweb maintainers and contributors alike have to do audits of
all routes that are available, to verify things like security and documentation
accuracy.  While these processes are largely manual, they can mostly be
conducted by a simple grep statement to search for urls in question.  Doing the
above makes the route handlers no longer greppable, and complicates bmcweb
patchsets as a whole.
