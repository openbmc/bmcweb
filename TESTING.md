# OpenBMC Webserver Testing

This doc decribes what type of testing, in addition to Unit Tests, which we
recommend contributors to perform before requesting code reviews, and tips for
each type. Contributors should put types of testing they performed and their
results to commit messages.

Depending the content of change list, not all types of tests are required.

## Basic IPMI and Redfish Robot Test on QEMU

Run
the [upstream Robot QEMU test](https://github.com/openbmc/openbmc-build-scripts/blob/master/run-qemu-robot-test.sh)
. This test is performed automatically when bumping SRCREV. Ensuring this test
passing makes your CL less likely to be rolled back while bumping SRCREV of
bmcweb.

```bash
# Clone the OpenBMC distribution
$ git clone https://github.com/openbmc/openbmc.git openbmc_distro

# build QEMU
$ export MACHINE=qemuarm
$ source setup ${MACHINE}
$ bitbake obmc-phosphor-image
$ bitbake build-sysroots
 
# build a target machine; e.g. romulus
$ export MACHINE=romulus
$ source setup ${MACHINE}
$ bitbake obmc-phosphor-image

# run the QEMU test; assume on an x86_64 desktop
$ UPSTREAM_WORKSPACE=/path/to/openbmc_distro/ \
QEMU_BIN=./qemuarm/tmp/sysroots/x86_64/usr/bin/qemu-system-arm \
DEFAULT_IMAGE_LOC=./build/${MACHINE}/tmp/deploy/images/ \
MACHINE=${MACHINE} \
/path/to/openbmc-build-scripts/run-qemu-robot-test.sh
```

## Websocket

Turn on any meson option that provides a websocket route, e.g., `rest`.

```bash
# run bmcweb on real hardware, QEMU, or desktop; here we run on desktop
$ meson -Drest=enabled -Dbmcweb-logging=enabled \
  -Dinsecure-disable-ssl=enabled build && ninja -C build
$ build/bmcweb

# run the websocket testing script and verify results
$ python scripts/websocket_test.py  --host localhost:18080 --no-ssl
```

## Redfish Validator

We recommend running
the [Redfish Validator](https://github.com/DMTF/Redfish-Service-Validator.git)
on real hardware since the resource tree will be more complete.

The following commands are just examples. Change your settings accordingly.

```bash
$ git clone https://github.com/DMTF/Redfish-Service-Validator.git

# verify service manually
$ wget -qO- --no-check-certificate  \
  --header='Authorization: Basic cm9vdDowcGVuQm1j' \
  https://${HOST}:${PORT}/redfish/v1/Chassis

# run validator and inspect the report
$ python RedfishServiceValidator.py \
  --auth Session -i https://${HOST}:${PORT} \
  -u root -p 0penBmc
```

## Error Status

Test error status for your newly added resources or core codes, e.g., 4xx client
errors, 5xx server errors.

## Push Stype Eventing

Create a subscription.

```bash
$ curl -v --request POST \
  --data '{"Protocol":"Redfish","Context":"Test_Context","Destination":"http://$LISTENER_FQDN:$LISTENER_PORT/","EventFormatType":"Event","SubscriptionType":"RedfishEvent","DeliveryRetryPolicy":"SuspendRetries","RegistryPrefixes":["OpenBMC"]}' \
  'http://${BMC}:${BMC_REDFISH_PORT}/redfish/v1/EventService/Subscriptions'

$ # see the created Subscription
curl --request GET 'http://${BMC}:${BMC_REDFISH_PORT}/redfish/v1/EventService/Subscriptions/'
{
  "@odata.id": "/redfish/v1/EventService/Subscriptions",
  "@odata.type": "#EventDestinationCollection.EventDestinationCollection",
  "Members": [
    {
      "@odata.id": "/redfish/v1/EventService/Subscriptions/1973135320"
    }
  ],
  "Members@odata.count": 1,
  "Name": "Event Destination Collections"
}

$ curl --request GET 'http://${BMC}:${BMC_REDFISH_PORT}/redfish/v1/EventService/Subscriptions/1973135320'
{
  "@odata.id": "/redfish/v1/EventService/Subscriptions/1973135320",
  "@odata.type": "#EventDestination.v1_7_0.EventDestination",
  "Context": "Test_Context",
  "DeliveryRetryPolicy": "SuspendRetries",
  "Destination": "http://xxxx:xxxx/",
  "EventFormatType": "Event",
  "HttpHeaders": [],
  "Id": "1973135320",
  "MessageIds": [],
  "MetricReportDefinitions": [],
  "Name": "Event Destination 1973135320",
  "Protocol": "Redfish",
  "RegistryPrefixes": [
    "OpenBMC"
  ],
  "ResourceTypes": [],
  "SubscriptionType": "RedfishEvent"
}
```

Spin up a dummy listener which prints all requests and replies OK without
contents.

```bash
$ python scripts/event_listener.py -i $LISTENER_FQDN -p $LISTENER_PORT
```

Create testing events.

```bash
$ curl --request POST \
  --data '{"OriginOfCondition":"/redfish/v1/Systems/system/LogServices/EventLog","EventId":"xxxx"}' \
  'http://${BMC}:${BMC_REDFISH_PORT}/redfish/v1/EventService/Actions/EventService.SubmitTestEvent'
```

Verify the listener gets events

```bash
POST request,
Path: /
Headers:
Host: a.b.c.d
Content-Length: 386

Body:
{
  "@odata.type": "#Event.v1_4_0.Event",
  "Events": [
    {
      "Context": "Test_Context",
      "EventId": "TestID",
      "EventTimestamp": "2022-02-24T00:00:12+00:00",
      "EventType": "Event",
      "Message": "Generated test event",
      "MessageArgs": [],
      "MessageId": "OpenBMC.0.2.TestEventLog",
      "Severity": "OK"
    }
  ],
  "Id": "3",
  "Name": "Event Log"
}
```
