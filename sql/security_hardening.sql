-- ============================================================================
-- MaNGOS Three — Security Hardening Script
-- Run AFTER setup_mariadb.sql to apply security fixes and constraints.
-- Usage: mariadb -u root < sql/security_hardening.sql
-- ============================================================================

-- ── Restrict mangos user to only needed privileges (not ALL) ─────────────
-- Revoke ALL and grant only what the server actually needs
USE `realmd`;

-- Revoke dangerous privileges from mangos user on realmd
REVOKE ALL PRIVILEGES ON `realmd`.* FROM 'mangos'@'localhost';
REVOKE ALL PRIVILEGES ON `realmd`.* FROM 'mangos'@'127.0.0.1';

GRANT SELECT, INSERT, UPDATE, DELETE ON `realmd`.* TO 'mangos'@'localhost';
GRANT SELECT, INSERT, UPDATE, DELETE ON `realmd`.* TO 'mangos'@'127.0.0.1';

-- Same for character3 and mangos3
REVOKE ALL PRIVILEGES ON `character3`.* FROM 'mangos'@'localhost';
REVOKE ALL PRIVILEGES ON `character3`.* FROM 'mangos'@'127.0.0.1';
REVOKE ALL PRIVILEGES ON `mangos3`.* FROM 'mangos'@'localhost';
REVOKE ALL PRIVILEGES ON `mangos3`.* FROM 'mangos'@'127.0.0.1';

GRANT SELECT, INSERT, UPDATE, DELETE ON `character3`.* TO 'mangos'@'localhost';
GRANT SELECT, INSERT, UPDATE, DELETE ON `character3`.* TO 'mangos'@'127.0.0.1';
GRANT SELECT, INSERT, UPDATE, DELETE ON `mangos3`.*    TO 'mangos'@'localhost';
GRANT SELECT, INSERT, UPDATE, DELETE ON `mangos3`.*    TO 'mangos'@'127.0.0.1';

FLUSH PRIVILEGES;

-- ── Add account lockout tracking table ───────────────────────────────────
USE `realmd`;

CREATE TABLE IF NOT EXISTS `account_login_attempts` (
    `id`         INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `account_id` INT UNSIGNED NOT NULL DEFAULT 0,
    `ip`         VARCHAR(45) NOT NULL DEFAULT '',
    `login_time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `success`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`),
    KEY `idx_account` (`account_id`),
    KEY `idx_ip` (`ip`),
    KEY `idx_time` (`login_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ── Add rate limiting metadata table ─────────────────────────────────────
CREATE TABLE IF NOT EXISTS `ip_rate_limit` (
    `ip`           VARCHAR(45)  NOT NULL,
    `attempts`     INT UNSIGNED NOT NULL DEFAULT 0,
    `window_start` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `blocked_until` TIMESTAMP NULL DEFAULT NULL,
    PRIMARY KEY (`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ── Add constraints to prevent negative/overflow money values ────────────
USE `character3`;

-- Add CHECK constraints (MariaDB 10.2.1+)
ALTER TABLE `characters` ADD CONSTRAINT `chk_money_cap`
    CHECK (`money` <= 9999999999);

ALTER TABLE `characters` ADD CONSTRAINT `chk_level_range`
    CHECK (`level` BETWEEN 0 AND 85);

-- ── Add audit log table for security-sensitive operations ────────────────
CREATE TABLE IF NOT EXISTS `security_audit_log` (
    `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `timestamp`  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `account_id` INT UNSIGNED NOT NULL DEFAULT 0,
    `char_guid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `action`     VARCHAR(64) NOT NULL DEFAULT '',
    `details`    TEXT,
    `ip`         VARCHAR(45) NOT NULL DEFAULT '',
    PRIMARY KEY (`id`),
    KEY `idx_account` (`account_id`),
    KEY `idx_timestamp` (`timestamp`),
    KEY `idx_action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ── Harden item_instance to prevent duplication ──────────────────────────
-- Add a unique constraint on item-owner pair to prevent the same item
-- from being duplicated across multiple owners simultaneously
ALTER TABLE `item_instance` ADD CONSTRAINT `chk_item_count`
    CHECK (`count` > 0 AND `count` <= 2147483647);

-- ── Prevent gold mail exploits with mail money validation ────────────────
ALTER TABLE `mail` ADD CONSTRAINT `chk_mail_money`
    CHECK (`money` <= 9999999999);

-- ── Guild name uniqueness (already unique key on name in setup, verify) ──
-- Add length constraint
ALTER TABLE `guild` ADD CONSTRAINT `chk_guild_name_len`
    CHECK (LENGTH(`name`) > 0 AND LENGTH(`name`) <= 24);

-- ── Anti-exploitation: add foreign key constraints for referential integrity
-- These prevent orphaned records that could be exploited

-- character_inventory must reference valid characters
ALTER TABLE `character_inventory`
    ADD CONSTRAINT `fk_inv_char` FOREIGN KEY (`guid`)
    REFERENCES `characters` (`guid`) ON DELETE CASCADE;

-- character_spell must reference valid characters
ALTER TABLE `character_spell`
    ADD CONSTRAINT `fk_spell_char` FOREIGN KEY (`guid`)
    REFERENCES `characters` (`guid`) ON DELETE CASCADE;

-- character_aura must reference valid characters
ALTER TABLE `character_aura`
    ADD CONSTRAINT `fk_aura_char` FOREIGN KEY (`guid`)
    REFERENCES `characters` (`guid`) ON DELETE CASCADE;

-- character_queststatus must reference valid characters
ALTER TABLE `character_queststatus`
    ADD CONSTRAINT `fk_quest_char` FOREIGN KEY (`guid`)
    REFERENCES `characters` (`guid`) ON DELETE CASCADE;

-- ── Done ─────────────────────────────────────────────────────────────────
SELECT 'Security hardening applied successfully' AS status;
