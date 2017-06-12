# Copyright (c) Benjamin Kietzman (github.com/bkietz)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gobject import MainLoop

bus_name = 'com.example.Sample'

class Example(dbus.service.Object):
  def __init__(self, connection, path):
    dbus.service.Object.__init__(self, connection, path)
    self._last_input = None

  @dbus.service.method(bus_name+'.Iface', in_signature='v', out_signature='s')
  def StringifyVariant(self, var):
    self.LastInputChanged(var)      # emits the signal
    return str(var)

  @dbus.service.signal(bus_name+'.Iface', signature='v')
  def LastInputChanged(self, var):
    # run just before the signal is actually emitted
    # just put "pass" if nothing should happen
    self._last_input = var

  @dbus.service.method(bus_name+'.Iface', in_signature='', out_signature='v')
  def GetLastInput(self):
    return self._last_input

bus = dbus.SessionBus(mainloop=DBusGMainLoop())
bus.request_name(bus_name)

example = Example(bus, '/path/to/obj')

print bus.get_name_owner(bus_name)
MainLoop().run()
