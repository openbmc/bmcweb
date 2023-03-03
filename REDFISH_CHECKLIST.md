When making changes to the Redfish tree, please include the following checklist.

Please mark each with:
Done Denoting this task was completed
WIP Denoting this task has not yet been completed
NA Denoting this task is not required for this change

[ ] I have verified the documentation in Redfish.md includes all properties
present in my commit.
[ ] I have documented any client-facing changes to the Redfish tree in my commit
message, including property additions, or changes in behavior per DEVELOPING.md
[ ] I have verified that the @odata.type on the schema on which I've added
functionality has the correct version.
[ ] I have verified that testing was performed per TESTING.md and it is
documented in my commit message.
[ ] I have read and understood COMMON\_ERRORS.md, and verified that to the best
of my knowledge no common errors are present in this patchset.
[ ] I have verified that all DBus usages match phosphor-dbus-interfaces, or I
have included a link to the gerrit review for phosphor-dbus-interfaces in my
commit message.
