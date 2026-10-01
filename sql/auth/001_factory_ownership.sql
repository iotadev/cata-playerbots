-- Optional mod-playerbots schema. Apply manually to the configured auth database.
-- This file creates no accounts, ownership records or characters, and is not
-- registered with the core updater. The feature remains default-off.
-- Console enrollment explicitly dedicates an existing empty account.
-- Never populate this table by scanning account/character name prefixes.
CREATE TABLE IF NOT EXISTS `playerbots_factory_ownership` (
  `account_id` INT UNSIGNED NOT NULL,
  `realm_id` INT UNSIGNED NOT NULL,
  `account_name` VARCHAR(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
  `account_join_epoch` BIGINT UNSIGNED NOT NULL,
  `evidence_version` INT UNSIGNED NOT NULL,
  `character_name` VARCHAR(12) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
  `race` TINYINT UNSIGNED NOT NULL,
  `class` TINYINT UNSIGNED NOT NULL,
  `gender` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`account_id`, `realm_id`),
  UNIQUE KEY `uq_playerbots_factory_account` (`account_id`),
  CONSTRAINT `fk_playerbots_factory_account` FOREIGN KEY (`account_id`)
    REFERENCES `account` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB;
