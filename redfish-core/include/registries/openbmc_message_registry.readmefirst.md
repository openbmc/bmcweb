Please read this before making edits to the openbmc base registry.

Messages in this registry are intended to be generally useful across OpenBMC
systems. To that end, there are a number of guidelines needed for messages in
this file to remain useful across systems.

1. Messages should not be specific to a piece or type of hardware. Generally
   this involves not using terms like "CPU", "Drive", "Backplane", etc in the
   message strings, unless that behavior is specific to that particular piece of
   hardware. Incorrect: MemoryOverheated Correct: DeviceOverheated Correct:
   MemoryECCError (ECC is specific to memory, therefore class specific is
   acceptable)

2. Messages should not use any proprietary, copyright, or company-specific
   terms. In general, messages should prefer the industry accepted term.

3. Message strings should be human readable, and read like a normal sentence.
   This generally involves placing the substitution parameters in the
   appropriate place in the string.

Incorrect: "An error occurred. Device: %1"

Correct: "An error occurred on device %1".

4. Message registry versioning semantics shall be obeyed. Adding new messages
   require an increment to the subminor revision. Changes to existing messages
   require an increment to the patch version. If the copyright year is different
   than the current date, increment it when the version is changed.

5. If you are changing this in your own downstream company-specific fork, please
   change the "id" field below away from "OpenBMC.0.X.X" to something of the
   form of "CompanyName.0.X.X". This is to ensure that when a system is found in
   industry that has modified this registry, it can be differentiated from the
   OpenBMC project maintained registry.
