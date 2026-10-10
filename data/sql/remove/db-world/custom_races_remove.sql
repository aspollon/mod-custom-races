-- mod-custom-races: run by AFK Realm when the module is removed (data/sql/remove).
-- The module writes the rows of its races at every start, and removes those of the races that are off; these are the
-- rows of all its races, by race id (Goblin 9) and race mask (Goblin 256).
DELETE FROM `chrraces_dbc` WHERE `ID` IN (9);
DELETE FROM `playercreateinfo` WHERE `race` IN (9);
DELETE FROM `playercreateinfo_action` WHERE `race` IN (9);
DELETE FROM `playercreateinfo_item` WHERE `race` IN (9);
DELETE FROM `player_race_stats` WHERE `Race` IN (9);
DELETE FROM `playercreateinfo_skills` WHERE `raceMask` IN (256);
DELETE FROM `playercreateinfo_spell_custom` WHERE `racemask` IN (256);
DELETE FROM `ascension_custom_class_race` WHERE `race` IN (9);
