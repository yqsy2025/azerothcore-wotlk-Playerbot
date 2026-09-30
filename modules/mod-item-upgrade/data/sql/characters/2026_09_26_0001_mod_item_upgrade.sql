-- mod-item-upgrade
-- Character DB
-- Only run this CREATE if your server does not already have _item_upgrade.

CREATE TABLE IF NOT EXISTS `_item_upgrade` (
  `instance_guid` INT UNSIGNED NOT NULL,
  `upgrade_lvl` INT UNSIGNED NOT NULL DEFAULT 0,
  `owner_guid` INT UNSIGNED NOT NULL DEFAULT 0,
  `increase_ratio` INT UNSIGNED NOT NULL DEFAULT 100,
  PRIMARY KEY (`instance_guid`),
  KEY `idx_owner_guid` (`owner_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
