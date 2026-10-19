-- mod-custom-races: the objects CoA Custom's racial abilities summon (from CoA Custom 1.8, files/sql/10_esteria_racials_1.8.sql):
-- Holo's decoy (creature 9100001) and Mastercraft's invisible anvil and forge (9100101, 9100102). The invisible cooking
-- fire (184911) loses its script, as on CoA Custom, so the one Mastercraft summons stays. AFK Realm records these rows
-- and takes them back out when the module is removed.
DELETE FROM `creature_template` WHERE `entry` IN (9100001);
INSERT INTO `creature_template` (`entry`,`difficulty_entry_1`,`difficulty_entry_2`,`difficulty_entry_3`,`KillCredit1`,`KillCredit2`,`name`,`subname`,`IconName`,`gossip_menu_id`,`minlevel`,`maxlevel`,`exp`,`faction`,`npcflag`,`speed_walk`,`speed_run`,`speed_swim`,`speed_flight`,`detection_range`,`rank`,`dmgschool`,`DamageModifier`,`BaseAttackTime`,`RangeAttackTime`,`BaseVariance`,`RangeVariance`,`unit_class`,`unit_flags`,`unit_flags2`,`dynamicflags`,`family`,`type`,`type_flags`,`lootid`,`pickpocketloot`,`skinloot`,`PetSpellDataId`,`VehicleId`,`mingold`,`maxgold`,`AIName`,`MovementType`,`HoverHeight`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,`ExperienceModifier`,`RacialLeader`,`movementId`,`RegenHealth`,`CreatureImmunitiesId`,`flags_extra`,`ScriptName`,`VerifiedBuild`) VALUES
(9100001,0,0,0,0,0,'Organic Light','Hyper Organic Light Originator',NULL,0,1,1,0,35,0,1.0,1.14286,1.0,1.0,20.0,0,0,0.0,2000,2000,1.0,1.0,1,0,0,0,0,7,0,0,0,0,0,0,0,0,'',0,1.0,1.0,1.0,1.0,1.0,0,0,1,0,64,'',NULL);
DELETE FROM `creature_template_model` WHERE `CreatureID` IN (9100001);
INSERT INTO `creature_template_model` (`CreatureID`,`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`,`VerifiedBuild`) VALUES
(9100001,0,11686,1.0,1.0,NULL);
DELETE FROM `gameobject_template` WHERE `entry` IN (9100101,9100102);
INSERT INTO `gameobject_template` (`entry`,`type`,`displayId`,`name`,`IconName`,`castBarCaption`,`unk1`,`size`,`Data0`,`Data1`,`Data2`,`Data3`,`Data4`,`Data5`,`Data6`,`Data7`,`Data8`,`Data9`,`Data10`,`Data11`,`Data12`,`Data13`,`Data14`,`Data15`,`Data16`,`Data17`,`Data18`,`Data19`,`Data20`,`Data21`,`Data22`,`Data23`,`AIName`,`ScriptName`,`VerifiedBuild`) VALUES
(9100101,8,0,'Invisible Anvil Object','','','',1.0,1,10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'','',NULL),
(9100102,8,0,'Invisible Forge Object','','','',1.0,3,10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'','',NULL);
UPDATE gameobject_template SET ScriptName='' WHERE entry = 184911;
