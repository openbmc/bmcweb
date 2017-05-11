#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <boost/iostreams/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

/**
 * Connect to the DBUS bus and send a broadcast signal
 */
void sendsignal(char* sigvalue) {
  DBusMessage* msg;
  DBusMessageIter args;
  DBusConnection* conn;
  DBusError err;
  int ret;
  dbus_uint32_t serial = 0;

  printf("Sending signal with value %s\n", sigvalue);

  // initialise the error value
  dbus_error_init(&err);

  // connect to the DBUS system bus, and check for errors
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Connection Error (%s)\n", err.message);
    dbus_error_free(&err);
  }
  if (NULL == conn) {
    exit(1);
  }

  // register our name on the bus, and check for errors
  ret = dbus_bus_request_name(conn, "test.signal.source",
                              DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Name Error (%s)\n", err.message);
    dbus_error_free(&err);
  }
  if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
    exit(1);
  }

  // create a signal & check for errors
  msg = dbus_message_new_signal(
      "/test/signal/Object",  // object name of the signal
      "test.signal.Type",     // interface name of the signal
      "Test");                // name of the signal
  if (NULL == msg) {
    fprintf(stderr, "Message Null\n");
    exit(1);
  }

  // append arguments onto signal
  dbus_message_iter_init_append(msg, &args);
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &sigvalue)) {
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }

  // send the message and flush the connection
  if (!dbus_connection_send(conn, msg, &serial)) {
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  dbus_connection_flush(conn);

  printf("Signal Sent\n");

  // free the message and close the connection
  dbus_message_unref(msg);
  dbus_connection_close(conn);
}

/**
 * Call a method on a remote object
 */
void query(const char* param) {
  DBusMessage* msg;
  DBusMessageIter args;
  DBusConnection* conn;
  DBusError err;
  DBusPendingCall* pending;
  int ret;
  bool stat;
  dbus_uint32_t level;

  printf("Calling remote method with %s\n", param);

  // initialiset the errors
  dbus_error_init(&err);

  // connect to the system bus and check for errors
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Connection Error (%s)\n", err.message);
    dbus_error_free(&err);
  }
  if (NULL == conn) {
    exit(1);
  }
  /*
  // request our name on the bus
  ret = dbus_bus_request_name(conn, "test.method.caller",
  DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
  if (dbus_error_is_set(&err)) {
     fprintf(stderr, "Name Error (%s)\n", err.message);
     dbus_error_free(&err);
  }
  if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
     exit(1);
  }
  */

  // create a new method call and check for errors
  msg = dbus_message_new_method_call(
      "org.freedesktop.Avahi",         // target for the method call
      "/",                             // object to call on
      "org.freedesktop.Avahi.Server",  // interface to call on
      "GetHostName");                  // method name
  if (NULL == msg) {
    fprintf(stderr, "Message Null\n");
    exit(1);
  }

  // append arguments
  /*
  dbus_message_iter_init_append(msg, &args);
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &param)) {
     fprintf(stderr, "Out Of Memory!\n");
     exit(1);
  }
  */
  // send message and get a handle for a reply
  if (!dbus_connection_send_with_reply(conn, msg, &pending,
                                       -1)) {  // -1 is default timeout
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  if (NULL == pending) {
    fprintf(stderr, "Pending Call Null\n");
    exit(1);
  }
  dbus_connection_flush(conn);

  printf("Request Sent\n");

  // free message
  dbus_message_unref(msg);

  // block until we recieve a reply
  dbus_pending_call_block(pending);

  // get the reply message
  msg = dbus_pending_call_steal_reply(pending);
  if (NULL == msg) {
    fprintf(stderr, "Reply Null\n");
    exit(1);
  }
  // free the pending message handle
  dbus_pending_call_unref(pending);

  // read the parameters
  char* str = NULL;
  if (!dbus_message_iter_init(msg, &args))
    fprintf(stderr, "Message has no arguments!\n");
  else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
    fprintf(stderr, "Argument is not boolean!\n");
  else
    dbus_message_iter_get_basic(&args, &str);

  printf("Got Reply: %s\n", str);

  // free reply and close connection
  dbus_message_unref(msg);
  dbus_connection_close(conn);
}

void list_names() {
  DBusError err;

  int ret;
  bool stat;
  dbus_uint32_t level;

  // initialiset the errors
  dbus_error_init(&err);

  // connect to the system bus and check for errors
  DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Connection Error (%s)\n", err.message);
    dbus_error_free(&err);
  }
  if (NULL == conn) {
    exit(1);
  }

  // create a new method call and check for errors
  DBusMessage* msg = dbus_message_new_method_call(
      "org.freedesktop.DBus",  // target for the method call
      "/",                     // object to call on
      "org.freedesktop.DBus",  // interface to call on
      "ListNames");            // method name
  if (NULL == msg) {
    fprintf(stderr, "Message Null\n");
    exit(1);
  }

  DBusPendingCall* pending;
  // send message and get a handle for a reply
  if (!dbus_connection_send_with_reply(conn, msg, &pending,
                                       -1)) {  // -1 is default timeout
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  if (NULL == pending) {
    fprintf(stderr, "Pending Call Null\n");
    exit(1);
  }
  dbus_connection_flush(conn);

  // free message
  dbus_message_unref(msg);

  // block until we recieve a reply
  dbus_pending_call_block(pending);

  // get the reply message
  msg = dbus_pending_call_steal_reply(pending);
  if (NULL == msg) {
    fprintf(stderr, "Reply Null\n");
    exit(1);
  }
  // free the pending message handle
  dbus_pending_call_unref(pending);

  // read the parameters
  DBusMessageIter args;
  DBusMessageIter strings;
  char* paths = NULL;
  if (!dbus_message_iter_init(msg, &args)) {
    fprintf(stderr, "Message has no arguments!\n");
  }
  std::vector<std::string> names;
  do {
    dbus_message_iter_recurse(&args, &strings);
    do {
      dbus_message_iter_get_basic(&strings, &paths);
      names.emplace_back(paths);
    } while (dbus_message_iter_next(&strings));
  } while (dbus_message_iter_next(&args));

  // free reply and close connection
  dbus_message_unref(msg);
  dbus_connection_close(conn);
}

std::vector<std::string> read_dbus_xml_names(std::string& xml_data) {
  std::vector<std::string> values;
  // populate tree structure pt
  using boost::property_tree::ptree;
  ptree pt;
  boost::iostreams::stream<boost::iostreams::array_source> stream(
      xml_data.c_str(), xml_data.size());
  read_xml(stream, pt);

  // traverse node to find other nodes
  for (const auto& interface : pt.get_child("node")) {
    if (interface.first == "node") {
      auto t = interface.second.get<std::string>("<xmlattr>", "default");
      for (const auto& subnode : interface.second.get_child("<xmlattr>")) {
        if (subnode.first == "name") {
          auto t = subnode.second.get("", "unknown");
          values.emplace_back(std::move(t));
        }
      }
    }
  }
  return values;
}

using sensor_values=std::vector<std::pair<std::string, int32_t>>;

sensor_values read_sensor_values() {
  sensor_values values;
  DBusError err;

  int ret;
  bool stat;
  dbus_uint32_t level;

  // initialiset the errors
  dbus_error_init(&err);

  // connect to the system bus and check for errors
  DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Connection Error (%s)\n", err.message);
    dbus_error_free(&err);
  }
  if (NULL == conn) {
    exit(1);
  }

  // create a new method call and check for errors
  DBusMessage* msg = dbus_message_new_method_call(
      "org.openbmc.Sensors",                  // target for the method call
      "/org/openbmc/sensors/tach",            // object to call on
      "org.freedesktop.DBus.Introspectable",  // interface to call on
      "Introspect");                          // method name
  if (NULL == msg) {
    fprintf(stderr, "Message Null\n");
    exit(1);
  }

  DBusPendingCall* pending;
  // send message and get a handle for a reply
  if (!dbus_connection_send_with_reply(conn, msg, &pending,
                                       -1)) {  // -1 is default timeout
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  if (NULL == pending) {
    fprintf(stderr, "Pending Call Null\n");
    exit(1);
  }
  dbus_connection_flush(conn);

  // free message
  dbus_message_unref(msg);

  // block until we recieve a reply
  dbus_pending_call_block(pending);

  // get the reply message
  msg = dbus_pending_call_steal_reply(pending);
  if (NULL == msg) {
    fprintf(stderr, "Reply Null\n");
    exit(1);
  }
  // free the pending message handle
  dbus_pending_call_unref(pending);

  // read the parameters
  DBusMessageIter args;
  char* xml_struct = NULL;
  if (!dbus_message_iter_init(msg, &args)) {
    fprintf(stderr, "Message has no arguments!\n");
  }

  // read the arguments
  if (!dbus_message_iter_init(msg, &args)) {
    fprintf(stderr, "Message has no arguments!\n");
  } else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) {
    fprintf(stderr, "Argument is not string!\n");
  } else {
    dbus_message_iter_get_basic(&args, &xml_struct);
  }
  std::vector<std::string> methods;
  if (xml_struct != NULL) {
    std::string xml_data(xml_struct);
    methods = read_dbus_xml_names(xml_data);
  }

  fprintf(stdout, "Found %ld sensors \n", methods.size());

  for (auto& method : methods) {
    // TODO(Ed) make sure sensor exposes SensorValue interface
    // create a new method call and check for errors
    DBusMessage* msg = dbus_message_new_method_call(
        "org.openbmc.Sensors",                  // target for the method call
        ("/org/openbmc/sensors/tach/" + method).c_str(),  // object to call on
        "org.openbmc.SensorValue",              // interface to call on
        "getValue");                            // method name
    if (NULL == msg) {
      fprintf(stderr, "Message Null\n");
      exit(1);
    }

    DBusPendingCall* pending;
    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply(conn, msg, &pending,
                                         -1)) {  // -1 is default timeout
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
    }
    if (NULL == pending) {
      fprintf(stderr, "Pending Call Null\n");
      exit(1);
    }
    dbus_connection_flush(conn);

    // free message
    dbus_message_unref(msg);

    // block until we recieve a reply
    dbus_pending_call_block(pending);

    // get the reply message
    msg = dbus_pending_call_steal_reply(pending);
    if (NULL == msg) {
      fprintf(stderr, "Reply Null\n");
      exit(1);
    }
    // free the pending message handle
    dbus_pending_call_unref(pending);

    // read the parameters
    DBusMessageIter args;
    int32_t value;
    if (!dbus_message_iter_init(msg, &args)) {
      fprintf(stderr, "Message has no arguments!\n");
    }

    // read the arguments
    if (!dbus_message_iter_init(msg, &args)) {
      fprintf(stderr, "Message has no arguments!\n");
    } else if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&args)) {
      fprintf(stderr, "Argument is not string!\n");
    } else {
      DBusMessageIter sub;
      dbus_message_iter_recurse(&args, &sub);
      auto type = dbus_message_iter_get_arg_type(&sub);
      if (DBUS_TYPE_INT32 != type) {
        fprintf(stderr, "Variant subType is not int32 it is %d\n", type);
      } else {
        dbus_message_iter_get_basic(&sub, &value);
        values.emplace_back(method.c_str(), value);
      }
    }
  }

  // free reply and close connection
  dbus_message_unref(msg);
  return values;
}


int main(int argc, char** argv) {
  auto values = read_sensor_values();

  for (auto value: values){
    std::cout << value.first << ": " << value.second << "\n";
  }

  return 0;
}