# Client overview

bmcweb being a user and network facing daemon, is subject to a vast array of
tests and clients that could target it. The below attempts to provide a
non-exhaustive list of tests and clients that bmcweb is expected to be
compatible with, they are split into a couple categories. Entries in the test
category are intended to be qualification tests to ensure the bmcweb meets the
specification. Entries in the clients category are intended to host user-facing
functionality.

The base expectation is that for master versions of bmcweb, and master versions
of these tools, the tests pass 100%. There may be cases where we workaround
limitations in the testing tools behavior within bmcweb to make the tools pass,
regardless of whether there is user impact.

## Tests

Redfish-Service-Validator: A tool to verify through GET requests that bmcweb
properly implements the Redfish schemas in the responded Resource.
git@github.com:DMTF/Redfish-Service-Validator.git

Status: 100% passing. Integrated with CI to ensure no regressions.

Redfish-Protocol-Validator: A tool to verify the protocol-level interactions
with the Redfish wire-protocol, and checks those against the Redfish
specification. git@github.com:DMTF/Redfish-Protocol-Validator.git

Status: 100% of assertions passing. No CI integration.

OpenBMC-test-automation: A tool based on robot framework for testing some
portion of the OpenBMC Redfish use cases.

Status: Passing for some machines with CI integration.

slowloris: A tool to verify timeouts and DOS attack mitigation is implemented
properly. <https://github.com/gkbrk/slowloris>

Status: Passing, no automated enforcement.

testssl.sh: A tool for verifying the corectness of the bmcweb cipher suites
against current recommended security standards
<https://github.com/drwetter/testssl.sh>

Status: Unknown

## Clients

fwupd: Is a client implementation that allows updating firmware components in a
generic way, and includes redfish as one of its plugins.
git@github.com:fwupd/fwupd.git

Status: Unknown

python-redfish-library: A python library used by a number of tools.
git@github.com:DMTF/python-redfish-library.git

Status: Compatible

Redfish-Event-Listener: An example client for testing and implementing
EventService handlers. <https://github.com/DMTF/Redfish-Event-Listener>

Status: Compatible. No CI integration.

Redfish-Tacklebox: A collection of common utilities for managing redfish servers
git@github.com:DMTF/Redfish-Tacklebox.git

Status: Unknown.

redfishtool: A generic command line tool for reading and writing operations to
the Redfish server. <https://github.com/DMTF/Redfishtool>

Status: Compatible. No automated testing.
