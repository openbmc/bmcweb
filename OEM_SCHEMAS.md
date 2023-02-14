# Redfish OEM Schemas

The Redfish specification allows for OEM resources and properties to be
implemented by OEMs. bmcweb does not expose a stable API for adding OEM
properties in a backward API compatible way for code that has not been merged to
master.

## OEM Compatibility and authority
Generally, a single individual or group of senior individuals in a corporate
organization is responsible for maintaining that company's OEM namespace. They
ensure that it remains correct, doesn't duplicate functionality found elsewhere,
and can be maintained forever.  Within OpenBMC, we have no such group of
individuals with that authority, knowledge, willpower, and scope that covers the
entire project.

Because of that, OEM properties in an open-source project pose many problems
when compared to their closed source brethren.


OpenBMC's external Redfish API aims to be as compatible between systems as
possible. Adding machine-specific resources, properties, and types defeats a
large amount of reuse, as clients must implement machine-specific APIs, some of
which are likely to overlap, which increases the amount of code overall. OpenBMC
also has very little visibility into clients that might interface with Redfish,
and therefore needs to take care when adding new, non-standard APIs, given the
lack of compatibility rules in such a case.

In the experience of the project, OEM resources trend toward a lower level of
quality and testing than their spec-driven alternatives, given the lack of
available systems to test on, and the limited audience of both producers and
consumers. This poses a problem for maintenance, as it is very difficult to make
a breaking change to an external API, given that clients are likely to be
implemented in projects that OpenBMC isn't aware of.

If a given feature eventually becomes standardized, OpenBMC OEM endpoints now
have to break an API boundary to move to the standard implementation.  Given the
effort it takes to break an API, it is much simpler to wait for the standard to
be completed before merging the OEM code to master.

DMTF has many more Redfish experts than OpenBMC. While the bmcweb maintainers do
their best to stay current on the evolving Redfish ecosystem, we have
significantly limited scope, knowledge, and influence over the standard when
compared to the experts within DMTF. Getting a DMTF opinion almost always leads
to positive API design changes up front, which increases the usefulness of the
code we write within the industry.

In the current implementation, OEM schemas for all namespaces are shipped on all
systems. It's undesirable to have another company's, possibly a competitor, name
show up in the public facing API as it exports a level of support that doesn't
exist on those systems.

## How do I write an OEM schema?

If you've read the above, and still think an OEM property is warranted, please
take the following steps.

1. Read all the relevant documentation on OEM schema technical implementation
   present in the Redfish specification. This includes examples of schemas that
   can be used as a template.
2. Present the new feature and use case to DMTF either through the
   [Redfish forum](https://www.redfishforum.com), or at a DMTF meeting. If
   possible, message the new feature through the normal openbmc communications
   channels to ensure OpenBMC is properly represented in the meeting/forum.
3. If DMTF is interested in the proposal, proceed using their documented process
   for change requests, and get your schema changes standardized; While OpenBMC
   does not merge new schemas that have not been ratified by DMTF, feel free to
   push them to gerrit. Maintainers are tasked with doing their best to
   accommodate active development; OEM schemas are no different. If the DMTF
   feedback is documented as something to the effect of "this use case is unique
   to OpenBMC", proceed to write an OpenBMC design document about the new
   feature you intend to implement as OEM, under the OpenBMC (generic to all
   platforms) OEM namespace.
4. If OpenBMC feedback is that this feature is specific to a single OEM or ODM,
   and is unlikely to be used across platforms, then engage with bmcweb
   maintainers, and they will walk you through how to develop the feature under
   an OEM/ODM specific namespace.

Regardless of the OEM namespace being used, implementations should plan to
implement all appropriate CSDL and OpenAPI schemas for their given OEM
resources, should pass the redfish service validator, should pass the csdl
validator and should follow redfish API design practices. We require OEM to have
the same level of quality as non-OEM.

bmcweb maintainers retain the final approval on OEM schemas.

The OpenBMC project maintains OEM schemas within the OpenBMC namespace, which,
from section 9.8.1 of the Redfish specification states:

''' There are organizations for which DMTF has a working relationship, and have
registered their OEM namespace directly in the specification to allow extensions
of the ICANN domain name requirements above. The following organization OEM
namespaces shall be considered reserved: OpenBMC '''

To avoid versioning complications with clients, schemas within the OpenBMC
namespace should not be modified without appropriate versioning information.
Given the nature of semantic versioning, Redfish does not directly have support
for schema branching within a namespace, therefore, if a system intends to ship
a version of the schemas that have been modified from the version available at
https://github.com/openbmc/bmcweb/tree/master/static/redfish/v1/schema, the
Redfish specification _requires_ that the namespace be changed to avoid
collisions.
