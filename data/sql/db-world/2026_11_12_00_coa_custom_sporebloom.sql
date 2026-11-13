-- mod-custom-races: the Sporebloom the Fungarian racial Sporebloom Circle (15000934) plants (from CoA Custom 1.9.9,
-- files/sql/23_racials_round2_v12.sql): the Venomancer mushroom (506018) without its script; it casts the pulse
-- (15000935) every 2 seconds.
DELETE FROM creature_template WHERE entry = 15000930;
DELETE FROM creature_template_model WHERE CreatureID = 15000930;
CREATE TEMPORARY TABLE tmp_sb SELECT * FROM creature_template WHERE entry = 506018;
UPDATE tmp_sb SET entry = 15000930, name = 'Sporebloom', ScriptName = '', AIName = 'SmartAI', unit_flags = 33555206, flags_extra = 0, speed_walk = 0, speed_run = 0;
INSERT INTO creature_template SELECT * FROM tmp_sb;
DROP TEMPORARY TABLE tmp_sb;
DELETE FROM smart_scripts WHERE entryorguid = 15000930 AND source_type = 0;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param6, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, target_type, target_param1, target_param2, target_param3, target_param4, target_x, target_y, target_z, target_o, comment) VALUES (15000930, 0, 0, 0, 1, 0, 100, 0, 1000, 1000, 2000, 2000, 0, 0, 11, 15000935, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Sporebloom - bloom pulse every 2 sec');
INSERT INTO creature_template_model (CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild) SELECT 15000930, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild FROM creature_template_model WHERE CreatureID = 506018;
