When making changes to the Redfish tree, please follow the checklist below.

If any of these are incomplete when pushing a patch to gerrit, either mark the
patch WIP (Work in progress), or explicitly include the items from the
checklist that have not been completed.

1. Verify the documentation in Redfish.md includes all properties and schemas that
have been added or are used in the commit.

2. Document any client-facing changes in behavior in the commit message.  Note,
that this should include behavior changes that may not effect your system.

3. Verify that any additional properties exist in the CSDL schema in
static/redfish/v1/odata.  DO NOT rely solely on the Json schema files, as they
are less restrictive than odata.

4. Document in the commit message what these property additions are used for.
Verify that the @odata.type on the schema on which you've added
functionality has a version that includes the parameters you've added

5. Verify that testing was performed per TESTING.md and it is
documented in my commit message.  Redfish-Service-Validator is the MINIMUM
required for any Redfish change.  Most changes require more tests than simply
service validator.  If testing was performed on a previous commit in the
series, but not the present commit, note that explicitly with "Tested on prior
commit".

6. Ensure that you have written unit tests for any behavior that does not have
external dependencies (Dbus).

7. Read COMMON\_ERRORS.md, and verify that to the best your knowledge no common
errors are present in the patchset.

8. Verify that all DBus usages match phosphor-dbus-interfaces, or include a link
to the gerrit review for phosphor-dbus-interfaces in the commit message.
