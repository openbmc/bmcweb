Boost D-Bus
===========

This is a simple D-Bus binding powered by Boost.Asio.
As far as possible, I try to follow Asio's idioms.

Code Sample
-----------

```c++
#include <iostream>

#include <boost/asio.hpp>
#include <dbus.hpp>

using namespace std;
using namespace boost::asio;
using boost::system::error_code;

struct logger
{
  void operator()(error_code ec, message m)
  {
	cout << m << endl;
  }
};

void main()
{
  io_service io;
  dbus::proxy avahi(io,
    dbus::endpoint(
	"org.freedesktop.Avahi", // proxied object process
	"/",                     // proxied object path
	"org.freedesktop.Avahi.Server")); // interface
						       
  dbus::message browser_spec(-1, -1,
    "_http._tcp", "local", unsigned(0));

  dbus::message response = 
    avahi.call("ServiceBrowserNew", browser_spec);

  dbus::proxy browser(io,
    dbus::endpoint(
	"org.freedesktop.Avahi",
	response.get(0),
	"org.freedesktop.Avahi.ServiceBrowser"));

  browser.async_receive("ItemNew", logger());

  io.run();
}


```
