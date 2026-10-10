-- mod-custom-races: run by AFK Realm when the module is removed (data/sql/remove).
-- Characters of its races play their base race from now on (Goblin: Orc). Their race and looks are kept aside in
-- `custom_races_moved`; the module, installed again, gives them back.
CREATE TABLE IF NOT EXISTS `custom_races_moved` (
  `guid` INT UNSIGNED NOT NULL,
  `race` TINYINT UNSIGNED NOT NULL,
  `base` TINYINT UNSIGNED NOT NULL,
  `skin` TINYINT UNSIGNED NOT NULL,
  `face` TINYINT UNSIGNED NOT NULL,
  `hairStyle` TINYINT UNSIGNED NOT NULL,
  `hairColor` TINYINT UNSIGNED NOT NULL,
  `facialStyle` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-custom-races: characters of a race switched off, and their looks';

REPLACE INTO `custom_races_moved` (`guid`, `race`, `base`, `skin`, `face`, `hairStyle`, `hairColor`, `facialStyle`)
SELECT `guid`, `race`, 2, `skin`, `face`, `hairStyle`, `hairColor`, `facialStyle` FROM `characters` WHERE `race` = 9;
UPDATE `characters` SET `race` = 2, `skin` = 0, `face` = 0, `hairStyle` = 0, `hairColor` = 0, `facialStyle` = 0 WHERE `race` = 9;
