-- The looks a race keeps beyond the five bytes of the stock packets (Highmountain Tauren and Earthen a sixth byte,
-- Haranir a 64-bit number), for EsteriaAppearance.dll of the game client. The core patch of mod-custom-races reads and
-- writes this column for every character, so it is added here once (the same column CoA Custom's server uses).
SET @has := (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE()
             AND TABLE_NAME = 'characters' AND COLUMN_NAME = 'extraAppearance');
SET @sql := IF(@has = 0, 'ALTER TABLE `characters` ADD COLUMN `extraAppearance` BIGINT UNSIGNED NOT NULL DEFAULT 0', 'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
