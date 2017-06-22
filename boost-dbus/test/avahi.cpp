// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>
#include <dbus/utility.hpp>
#include <functional>

#include <unistd.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(AvahiTest, GetHostName) {
  dbus::endpoint test_daemon("org.freedesktop.Avahi", "/",
                             "org.freedesktop.Avahi.Server");
  boost::asio::io_service io;
  dbus::connection system_bus(io, dbus::bus::system);

  dbus::message m = dbus::message::new_call(test_daemon, "GetHostName");

  system_bus.async_send(
      m, [&](const boost::system::error_code ec, dbus::message r) {

        std::string avahi_hostname;
        std::string hostname;

        // get hostname from a system call
        char c[1024];
        gethostname(c, 1024);
        hostname = c;

        r.unpack(avahi_hostname);

        // Get only the host name, not the fqdn
        auto unix_hostname = hostname.substr(0, hostname.find("."));
        EXPECT_EQ(unix_hostname, avahi_hostname);

        io.stop();
      });
  boost::asio::deadline_timer t(io, boost::posix_time::seconds(10));
  t.async_wait([&](const boost::system::error_code& /*e*/) {
    io.stop();
    FAIL() << "Callback was never called\n";
  });
  io.run();
}

TEST(AvahiTest, ServiceBrowser) {
  boost::asio::io_service io;
  dbus::connection system_bus(io, dbus::bus::system);

  dbus::endpoint test_daemon("org.freedesktop.Avahi", "/",
                             "org.freedesktop.Avahi.Server");
  // create new service browser
  dbus::message m1 = dbus::message::new_call(test_daemon, "ServiceBrowserNew");
  m1.pack<int32_t>(-1)
      .pack<int32_t>(-1)
      .pack<std::string>("_http._tcp")
      .pack<std::string>("local")
      .pack<uint32_t>(0);

  dbus::message r = system_bus.send(m1);
  std::string browser_path;
  r.unpack(browser_path);
  testing::Test::RecordProperty("browserPath", browser_path);

  dbus::match ma(system_bus, "type='signal',path='" + browser_path + "'");
  dbus::filter f(system_bus, [](dbus::message& m) {
    auto member = m.get_member();
    return member == "NameAcquired";
  });

  std::function<void(boost::system::error_code, dbus::message)> event_handler =
      [&](boost::system::error_code ec, dbus::message s) {
        testing::Test::RecordProperty("firstSignal", s.get_member());
        std::string a = s.get_member();
        std::string dude;
        s.unpack(dude);
        f.async_dispatch(event_handler);
        io.stop();
      };
  f.async_dispatch(event_handler);

  boost::asio::deadline_timer t(io, boost::posix_time::seconds(10));
  t.async_wait([&](const boost::system::error_code& /*e*/) {
    io.stop();
    FAIL() << "Callback was never called\n";
  });
  io.run();
}

TEST(BOOST_DBUS, ListServices) {
  boost::asio::io_service io;
  boost::asio::deadline_timer t(io, boost::posix_time::seconds(10));
  t.async_wait([&](const boost::system::error_code& /*e*/) {
    io.stop();
    FAIL() << "Callback was never called\n";
  });

  dbus::connection system_bus(io, dbus::bus::system);

  dbus::endpoint test_daemon("org.freedesktop.DBus", "/",
                             "org.freedesktop.DBus");
  // create new service browser
  dbus::message m = dbus::message::new_call(test_daemon, "ListNames");
  system_bus.async_send(
      m, [&](const boost::system::error_code ec, dbus::message r) {
        io.stop();
        std::vector<std::string> services;
        r.unpack(services);
        // Test a couple things that should always be present.... adapt if
        // neccesary
        EXPECT_THAT(services, testing::Contains("org.freedesktop.DBus"));
        EXPECT_THAT(services, testing::Contains("org.freedesktop.Accounts"));

      });

  io.run();
}

void query_interfaces(dbus::connection& system_bus, std::string& service_name,
                      std::string& object_name) {
  dbus::endpoint service_daemon(service_name, object_name,
                                "org.freedestop.DBus.Introspectable");
  dbus::message m = dbus::message::new_call(service_daemon, "Introspect");
  try {
    auto r = system_bus.send(m);
    std::vector<std::string> names;
    // Todo(ed) figure out why we're occassionally getting access
    // denied errors
    // EXPECT_EQ(ec, boost::system::errc::success);

    std::string xml;
    r.unpack(xml);
    // TODO(ed) names needs lock for multithreaded access
    dbus::read_dbus_xml_names(xml, names);
    // loop over the newly added items
    for (auto name : names) {
      std::cout << name << "\n";
      auto new_service_string = object_name + "/" + name;
      query_interfaces(system_bus, service_name, new_service_string);
    }
  } catch (boost::system::error_code e) {
    std::cout << e;
  }
}

TEST(BOOST_DBUS, SingleSensorChanged) {
  boost::asio::io_service io;
  dbus::connection system_bus(io, dbus::bus::system);

  dbus::match ma(system_bus,
                 "type='signal',path_namespace='/xyz/openbmc_project/sensors'");
  dbus::filter f(system_bus, [](dbus::message& m) {
    auto member = m.get_member();
    return member == "PropertiesChanged";
  });

  // std::function<void(boost::system::error_code, dbus::message)> event_handler
  // =

  f.async_dispatch([&](boost::system::error_code ec, dbus::message s) {
    std::string object_name;
    EXPECT_EQ(s.get_path(),
              "/xyz/openbmc_project/sensors/temperature/LR_Brd_Temp");

    std::vector<std::pair<std::string, dbus::dbus_variant>> values;
    s.unpack(object_name).unpack(values);

    EXPECT_EQ(object_name, "xyz.openbmc_project.Sensor.Value");

    EXPECT_EQ(values.size(), 1);
    auto expected = std::pair<std::string, dbus::dbus_variant>("Value", 42);
    EXPECT_EQ(values[0], expected);

    io.stop();
  });

  dbus::endpoint test_endpoint(
      "org.freedesktop.Avahi",
      "/xyz/openbmc_project/sensors/temperature/LR_Brd_Temp",
      "org.freedesktop.DBus.Properties");

  auto signal_name = std::string("PropertiesChanged");
  auto m = dbus::message::new_signal(test_endpoint, signal_name);

  m.pack("xyz.openbmc_project.Sensor.Value");

  std::vector<std::pair<std::string, dbus::dbus_variant>> map2;

  map2.emplace_back("Value", 42);

  m.pack(map2);

  auto removed = std::vector<uint32_t>();
  m.pack(removed);
  system_bus.async_send(m,
                        [&](boost::system::error_code ec, dbus::message s) {});

  io.run();
}

TEST(BOOST_DBUS, MultipleSensorChanged) {
  boost::asio::io_service io;
  dbus::connection system_bus(io, dbus::bus::system);

  dbus::match ma(system_bus,
                 "type='signal',path_namespace='/xyz/openbmc_project/sensors'");
  dbus::filter f(system_bus, [](dbus::message& m) {
    auto member = m.get_member();
    return member == "PropertiesChanged";
  });

  int count = 0;
  f.async_dispatch([&](boost::system::error_code ec, dbus::message s) {
    std::string object_name;
    EXPECT_EQ(s.get_path(),
              "/xyz/openbmc_project/sensors/temperature/LR_Brd_Temp");

    std::vector<std::pair<std::string, dbus::dbus_variant>> values;
    s.unpack(object_name).unpack(values);

    EXPECT_EQ(object_name, "xyz.openbmc_project.Sensor.Value");

    EXPECT_EQ(values.size(), 1);
    auto expected = std::pair<std::string, dbus::dbus_variant>("Value", 42);
    EXPECT_EQ(values[0], expected);
    count++;
    if (count == 2) {
      io.stop();
    }

  });

  dbus::endpoint test_endpoint(
      "org.freedesktop.Avahi",
      "/xyz/openbmc_project/sensors/temperature/LR_Brd_Temp",
      "org.freedesktop.DBus.Properties");

  auto signal_name = std::string("PropertiesChanged");
  auto m = dbus::message::new_signal(test_endpoint, signal_name);

  m.pack("xyz.openbmc_project.Sensor.Value");

  std::vector<std::pair<std::string, dbus::dbus_variant>> map2;

  map2.emplace_back("Value", 42);

  m.pack(map2);

  auto removed = std::vector<uint32_t>();
  m.pack(removed);
  system_bus.async_send(m,
                        [&](boost::system::error_code ec, dbus::message s) {});
  system_bus.async_send(m,
                        [&](boost::system::error_code ec, dbus::message s) {});
  io.run();
}