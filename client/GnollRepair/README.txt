Gnoll repair overlay for the CoA Custom client.

The CoA Custom 1.8.3 patches ZZT/ZZU/ZZV/ZZW have no (listfile), so name-based
tools cannot extract the Gnoll textures. This overlay is a clean MPQ made from the
Gnoll files named by the client tables and EsteriaGnoll.bin:

  custom\gnoll\native\gnoll.m2
  custom\gnoll\native\gnoll00.skin
  custom\gnoll\native\textures\*.blp
  Interface\Icons\CoA_Race_Gnoll.blp

It also carries a proper (listfile). AFK Realm copies it to Data\patch-ZZZZ.MPQ
when Ascension's HighRaces mode is on. It loads after ZZW/ZZZ and only overrides
those Gnoll files. Delete Data\patch-ZZZZ.MPQ (or switch HighRaces off) to remove
the workaround; the original CoA Custom patches are never modified.
