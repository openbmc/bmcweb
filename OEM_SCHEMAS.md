## Redfish OEM Resources

The Redfish specification allows for OEM resources and properties to be
implemented by OEMs.  As a general rule, OpenBMC discourages the use of OEM
namespaces in APIs.  In line with this, bmcweb does not expose an API for adding
OEM properties in a backward API compatible way for resources that have not been
merged to master.

### Why?
OEM properties in an open source project pose many problems when compared to
their closed source brethren in terms of reliability, code reuse, compatibility,
and maintenance.

1. As a general rule, OpenBMCs external Redfish API aims to be as compatible
   between systems as possible.  Adding machine-specific resources, properties,
   and types largely defeats some of that goal, as clients must implement
   machine-specific APIs, some of which are likely to overlap, which increases
   the amount of code overall.  OpenBMC also has very little visibility into
   clients that might interface with Redfish, and therefore needs to take care
   when adding new, non-standard APIs.

2. In practice, OEM resources trend toward a lower level of quality and testing
   than their spec-driven alternatives, given the lack of available systems to
   test on, and the limited audience of both producers and consumers. This poses
   a problem for maintenance in the long run, as it is very difficult to make a
   breaking change to an external API, given that clients are likely to be
   implemented in projects that OpenBMC isn't aware of.

3. If a given workflow eventually becomes standardized, OpenBMC OEM endpoints
   now have to break an API boundary to be able to move to the standard
   implementation.  Given the amount of effort it takes to break an API, it is
   much simpler to wait for the standard to be completed before merging the OEM
   code to master.

4. DMTF has many more Redfish experts than OpenBMC.  While the bmcweb
   maintainers do their best to stay current on the evolving Redfish ecosystem,
   we have significantly limited scope, knowledge, and influence over the
   standard when compared to the experts within DMTF.  Getting a DMTF opinion
   almost always leads to positive API design changes up front, which increases
   the usefulness of the code we write within the industry.

### How?

If you've read the above, and still think an OEM property is warranted, please
take the following steps.

1. Present the new feature and use case to DMTF either through the [Redfish
   forum](https://www.redfishforum.com), or at a DMTF meeting.  If possible,
   message the new feature through the normal openbmc communications channels to
   ensure OpenBMC is properly represented in the meeting/forum.
2. If DMTF is interested in the proposal, proceed using their documented process
   for change requests, and get your schema changes standardized;  While OpenBMC
   does not merge new schemas that have not been ratified by DMTF, feel free to
   push them to gerrit.  If the features are major enough to warrant it, feel
   free to discuss with maintainers about hosting a branch within gerrit while
   your DMTF proposal is in progress.  If the DMTF feedback is documented as
   something to the effect of "this use case is unique to OpenBMC", proceed to
   write an OpenBMC design document about the new feature you intend to
   implement as OEM, under the OpenBMC (generic to all platforms) OEM namespace.
3. If OpenBMC feedback is that this feature is specific to a single OEM or ODM,
   and is unlikely to be used across platforms, then maintainers will provide
   you the option of adding your feature under an OEM/ODM specific namespace.

Regardless of the OEM namespace being used, implementations should plan to
implement all appropriate CSDL and OpenAPI schemas for their given OEM
resources, should pass the redfish service validator, and should follow redfish
API design practices.  We require this same level of quality as non OEM to
ensure that OEM is truly required by the contributor to satisfy their use case.
If OEM were held to a lesser level of quality requirements, bwcweb would consist
entirely of OEM code.

bmcweb maintainers retain the final approval on OEM schemas.
