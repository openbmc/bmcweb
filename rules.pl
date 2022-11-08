submit_rule(S) :-
  gerrit:default_submit(D),
  D =.. [submit | Ds],
  findall(U, gerrit:commit_label(label('Code-Review', 2), U), Approvers),
  gerrit_owners:add_owner_approval(Approvers, Ds, A),
  S =.. [submit | A].
