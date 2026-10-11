-- mod-custom-races: run by AFK Realm when the module is removed (data/sql/remove). Written by tools/races.py.
-- Characters of its races play a stock race from now on: the base race the module gave them (kept in
-- `custom_races_base` while it ran, by the number each race had). Their race and looks are kept aside in
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

CREATE TABLE IF NOT EXISTS `custom_races_base` (
  `race` TINYINT UNSIGNED NOT NULL,
  `base` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`race`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-custom-races: the base race of each race';

REPLACE INTO `custom_races_moved` (`guid`, `race`, `base`, `skin`, `face`, `hairStyle`, `hairColor`, `facialStyle`)
SELECT c.`guid`, c.`race`, b.`base`, c.`skin`, c.`face`, c.`hairStyle`, c.`hairColor`, c.`facialStyle`
FROM `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race`;
UPDATE `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race` SET c.`race` = b.`base`, c.`skin` = 0, c.`face` = 0,
  c.`hairStyle` = 0, c.`hairColor` = 0, c.`facialStyle` = 0;
