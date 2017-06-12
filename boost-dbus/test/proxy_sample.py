# Copyright (c) Benjamin Kietzman (github.com/bkietz)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import dbus
from dbus.mainloop.glib import DBusGMainLoop
from gobject import MainLoop

bus_name = 'com.example.Sample'
session_bus = dbus.SessionBus(mainloop=DBusGMainLoop())

example_remote = session_bus.get_object(bus_name, '/path/to/obj')
example = dbus.Interface(example_remote, bus_name+'.Iface')

example.StringifyVariant(123)
print example.GetLastInput()

MainLoop().run()
