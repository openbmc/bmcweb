# Copyright (c) Benjamin Kietzman (github.com/bkietz)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import unittest
import dbus
from dbus.mainloop.glib import DBusGMainLoop
from gobject import MainLoop
from socket import gethostname

class AvahiTest(unittest.TestCase):

  @classmethod
  def setUpClass(c):
    c.system_bus = dbus.SystemBus(mainloop=DBusGMainLoop())

  def setUp(self):
    None

  def testAvahi(self):
    # Connect to Avahi Daemon's interface:
    avahi_remote = AvahiTest.system_bus.get_object('org.freedesktop.Avahi', '/')
    avahi = dbus.Interface(avahi_remote, 'org.freedesktop.Avahi.Server')
    self.assertEqual(gethostname(), avahi.GetHostName())

    # Use the Avahi Daemon to produce a new 
    # ServiceBrowser and connect to its interface:
    browser_path = avahi.ServiceBrowserNew(-1, -1, "_http._tcp", "local", dbus.UInt32(0))
    browser_remote = AvahiTest.system_bus.get_object('org.freedesktop.Avahi', browser_path)

    browser = dbus.Interface(browser_remote, 'org.freedesktop.Avahi.ServiceBrowser')

    # Connect to the ItemNew signal from the browser:
    def new_item_handler(interface, protocol, instance_name, instance_type, domain, flags):
      print "Found service '%s'" % instance_name

    browser.connect_to_signal("ItemNew", new_item_handler)

if __name__ == '__main__':
  unittest.main()
  MainLoop().run()
