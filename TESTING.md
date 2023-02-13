# OpenBMC Webserver Testing

This doc describes what type of functional testing, which contributors should
perform before requesting code reviews, and tips for each type. Contributors
should add types of testing they performed and their results as "Tested" footers
in commit messages.

## General Methodology

There are a variety of ways to develop and test bmcweb software changes. Here
are the steps for using the SDK and QEMU.

- Follow all
  [development environment setup](https://github.com/openbmc/docs/blob/master/development/dev-environment.md)
  directions in the development environment setup document. This will get QEMU
  started up and you in the SDK environment.
- Follow all of the
  [gerrit setup](https://github.com/openbmc/docs/blob/master/development/gerrit-setup.md)
  directions in the gerrit setup document.
- Clone bmcweb from gerrit

  ```sh
  git clone ssh://openbmc.gerrit/openbmc/bmcweb/
  ```

- Follow directions in
  [README.md](https://github.com/openbmc/bmcweb#configuration) to compile

- Reduce binary size by stripping it when ready for testing

  ```sh
  arm-openbmc-linux-gnueabi-strip bmcweb
  ```

  **Note:** Stripping is not required and having the debug symbols could be
  useful depending on your testing. Leaving them will drastically increase your
  transfer time to the BMC.

- Copy your bmcweb you want to test to /tmp/ in QEMU

  ```sh
  scp -P 2222 bmcweb root@127.0.0.1:/tmp/
  ```

  **Special Notes:** The address and port shown here (127.0.0.1 and 2222)
  reaches the QEMU session you set up in your development environment as
  described above.

- Stop bmcweb service within your QEMU session

  ```sh
  systemctl stop bmcweb
  ```

  **Note:** bmcweb supports being started directly in parallel with the bmcweb
  running as a service. The standalone bmcweb will be available on port 18080.
  An advantage of this is you can compare between the two easily for testing. In
  QEMU you would need to open up port 18080 when starting QEMU. Your curl
  commands would need to use 18080 to communicate.

- If running within a system that has read-only /usr/ filesystem, issue the
  following commands one time per QEMU boot to make the filesystem writeable

  ```sh
  mkdir -p /var/persist/usr
  mkdir -p /var/persist/work/usr
  mount -t overlay -o lowerdir=/usr,upperdir=/var/persist/usr,workdir=/var/persist/work/usr overlay /usr
  ```

- Remove the existing bmcweb from the filesystem in QEMU

  ```sh
  rm /usr/bin/bmcweb
  ```

- Link to your new bmcweb in /tmp/

  ```sh
  ln -sf /tmp/bmcweb /usr/bin/bmcweb
  ```

- Test your changes. bmcweb will be started automatically upon your first REST
  or Redfish command

  ```sh
  curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST https://127.0.0.1:2443/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
  curl -c cjar -b cjar -k -X GET https://127.0.0.1:2443/xyz/openbmc_project/state/bmc0
  ```

- Stop the bmcweb service and scp new file over to /tmp/ each time you want to
  retest a change.

  See the [REST](https://github.com/openbmc/docs/blob/master/REST-cheatsheet.md)
  and
  [Redfish](https://github.com/openbmc/docs/blob/master/REDFISH-cheatsheet.md)
  cheatsheets for valid commands.

## Types of Changes to Run

A committer should run tests that exercise all paths changed in the patchset. If
making changes to the http core, that are expected to effect all types of
routes, testing one route of each class (websocket, HTTP get, HTTP post) is
required.

## Typical Types of Changes

### Basic Redfish Robot Test on QEMU

Run the
[upstream Robot QEMU test](https://github.com/openbmc/openbmc-build-scripts/blob/master/run-qemu-robot-test.sh).
This test is performed automatically when bumping SRCREV. Ensuring this test
passing makes your CL less likely to be rolled back while bumping SRCREV of
bmcweb.

### Websocket

Turn on the `rest` meson option which provides a websocket route.

```bash
# run the websocket testing script and verify results
$ python scripts/websocket_test.py  --host 1.2.3.4:443 --ssl
```

### Redfish Validator

Commiters are required to run the
[Redfish Validator](https://github.com/DMTF/Redfish-Service-Validator.git)
anytime they make a change to the GET behavior of the redfish tree. The test
must run on real hardware since the resource tree will be more complete.

```bash
$ git clone https://github.com/DMTF/Redfish-Service-Validator.git

# run validator and inspect the report for failures
$ python3 RedfishServiceValidator.py \
  --auth Session -i https://1.2.3.4:443 \
  -u root -p 0penBmc
```

Your change should not introduce any new validator errors. Please include
something to the effect of "Redfish service validator passing" in your commit
message.

### Error Status

Test error status for your newly added resources or core codes, e.g., 4xx client
errors, 5xx server errors.
