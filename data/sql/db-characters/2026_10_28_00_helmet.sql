-- Characters whose helmet mark of an older mod-custom-races was taken away (it hid the helmets of races that have
-- them). Kept so it happens once: a player who hides the helmet himself afterwards keeps it hidden.
CREATE TABLE IF NOT EXISTS `custom_races_helmet` (
  `guid` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
