# OpenBMC Webserver Testing

This doc describes what type of testing, in addition to Unit Tests, which we
recommend contributors to perform before requesting code reviews, and tips for
each type. Contributors should add types of testing they performed and their
results as "Tested" footers in commit messages.

## General Methodology
There are a variety of ways to develop and test bmcweb software changes.
Here are the steps for using the SDK and QEMU.

- Follow all [development environment setup](https://github.com/openbmc/docs/blob/master/development/dev-environment.md)
directions in the development environment setup document. This will get
QEMU started up and you in the SDK environment.
- Follow all of the [gerrit setup](https://github.com/openbmc/docs/blob/master/development/gerrit-setup.md)
directions in the gerrit setup document.
- Clone bmcweb from gerrit
  ```
  git clone ssh://openbmc.gerrit/openbmc/bmcweb/
  ```

- Follow directions in [README.md](https://github.com/openbmc/bmcweb#configuration) to compile

- Reduce binary size by stripping it when ready for testing
  ```
  arm-openbmc-linux-gnueabi-strip bmcweb
  ```
  **Note:** Stripping is not required and having the debug symbols could be
  useful depending on your testing. Leaving them will drastically increase
  your transfer time to the BMC.

- Copy your bmcweb you want to test to /tmp/ in QEMU
  ```
  scp -P 2222 bmcweb root@127.0.0.1:/tmp/
  ```
  **Special Notes:**
  The address and port shown here (127.0.0.1 and 2222) reaches the QEMU session
  you set up in your development environment as described above.

- Stop bmcweb service within your QEMU session
  ```
  systemctl stop bmcweb
  ```
  **Note:** bmcweb supports being started directly in parallel with the bmcweb
  running as a service. The standalone bmcweb will be available on port 18080.
  An advantage of this is you can compare between the two easily for testing.
  In QEMU you would need to open up port 18080 when starting QEMU. Your curl
  commands would need to use 18080 to communicate.

- If running within a system that has read-only /usr/ filesystem, issue
the following commands one time per QEMU boot to make the filesystem
writeable
  ```
  mkdir -p /var/persist/usr
  mkdir -p /var/persist/work/usr
  mount -t overlay -o lowerdir=/usr,upperdir=/var/persist/usr,workdir=/var/persist/work/usr overlay /usr
  ```

- Remove the existing bmcweb from the filesystem in QEMU
  ```
  rm /usr/bin/bmcweb
  ```

- Link to your new bmcweb in /tmp/
  ```
  ln -sf /tmp/bmcweb /usr/bin/bmcweb
  ```

- Test your changes. bmcweb will be started automatically upon your
first REST or Redfish command
  ```
  curl -c cjar -b cjar -k -X POST https://127.0.0.1:2443/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
  curl -c cjar -b cjar -k -X GET https://127.0.0.1:2443/xyz/openbmc_project/state/bmc0
  ```

- Stop the bmcweb service and scp new file over to /tmp/ each time you
want to retest a change.

  See the [REST](https://github.com/openbmc/docs/blob/master/REST-cheatsheet.md)
  and [Redfish](https://github.com/openbmc/docs/blob/master/REDFISH-cheatsheet.md) cheatsheets for valid commands.


## Types of Changes to Run
Depending the content of change list, not all types of tests are required, but
you should run tests that exercise all paths through the code you changed. We
have the following suggestion for different kinds of changes.

1. Most changes: Cross Compilation, Robot Test on QEMU
2. HTTP core libraries: Cross Compilation, Robot Test on QEMU, Error Status, Websocket,
Redfish eventing, Redfish Validator
3. Individual resource: Cross Compilation, Robot Test on QEMU, Redfish Validator
on real hardware (that includes the changed resource)
4. Websocket: Websocket test

## Typical Types of Changes

### Cross Compilation
Patch the change and test cross compilation via Yocto. This catches many issues
that are hard to find during code review and won't be caught by CI on 64 bit
systems, e.g., recipe dependencies, compilation warnings, etc.

### Basic IPMI and Redfish Robot Test on QEMU

Run
the [upstream Robot QEMU test](https://github.com/openbmc/openbmc-build-scripts/blob/master/run-qemu-robot-test.sh)
. This test is performed automatically when bumping SRCREV. Ensuring this test
passing makes your CL less likely to be rolled back while bumping SRCREV of
bmcweb.

### Websocket

Turn on any meson option that provides a websocket route, e.g., `rest`.

```bash
# run bmcweb on real hardware to maximize the test coverage
$ meson -Drest=enabled -Dbmcweb-logging=enabled build && ninja -C build
$ build/bmcweb

# run the websocket testing script and verify results
$ python scripts/websocket_test.py  --host localhost:18080 --ssl
```

### Redfish Validator

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

Your change should not introduce any new validator errors. Please include
the Redfish Service Validator results as part of the commit message
"Tested" footers.

### Error Status

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
