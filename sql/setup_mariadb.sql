-- ============================================================================
-- MaNGOS Three — MariaDB Setup Script
-- Creates the three required databases, user, and core tables.
-- Usage: mariadb -u root < sql/setup_mariadb.sql
-- ============================================================================

-- ── Databases ────────────────────────────────────────────────────────────────
CREATE DATABASE IF NOT EXISTS `realmd`     DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
CREATE DATABASE IF NOT EXISTS `mangos3`    DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
CREATE DATABASE IF NOT EXISTS `character3` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;

-- ── User ─────────────────────────────────────────────────────────────────────
CREATE USER IF NOT EXISTS 'mangos'@'localhost' IDENTIFIED BY 'mangos';
CREATE USER IF NOT EXISTS 'mangos'@'127.0.0.1' IDENTIFIED BY 'mangos';
GRANT ALL PRIVILEGES ON `realmd`.*     TO 'mangos'@'localhost';
GRANT ALL PRIVILEGES ON `realmd`.*     TO 'mangos'@'127.0.0.1';
GRANT ALL PRIVILEGES ON `mangos3`.*    TO 'mangos'@'localhost';
GRANT ALL PRIVILEGES ON `mangos3`.*    TO 'mangos'@'127.0.0.1';
GRANT ALL PRIVILEGES ON `character3`.* TO 'mangos'@'localhost';
GRANT ALL PRIVILEGES ON `character3`.* TO 'mangos'@'127.0.0.1';
FLUSH PRIVILEGES;

-- ════════════════════════════════════════════════════════════════════════════
-- REALMD
-- ════════════════════════════════════════════════════════════════════════════
USE `realmd`;

CREATE TABLE IF NOT EXISTS `account` (
    `id`              INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `username`        VARCHAR(32)  NOT NULL DEFAULT '',
    `sha_pass_hash`   VARCHAR(40)  NOT NULL DEFAULT '',
    `gmlevel`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `sessionkey`      LONGTEXT,
    `v`               LONGTEXT,
    `s`               LONGTEXT,
    `email`           TEXT NOT NULL,
    `joindate`        TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `last_ip`         VARCHAR(30)  NOT NULL DEFAULT '127.0.0.1',
    `failed_logins`   INT UNSIGNED NOT NULL DEFAULT 0,
    `locked`          TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `last_login`      TIMESTAMP NOT NULL DEFAULT '0000-00-00 00:00:00',
    `active_realm_id` INT UNSIGNED NOT NULL DEFAULT 0,
    `expansion`       TINYINT UNSIGNED NOT NULL DEFAULT 3,
    `mutetime`        BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `locale`          TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `os`              VARCHAR(4)   NOT NULL DEFAULT '',
    `playerBot`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`),
    UNIQUE KEY `idx_username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `realmlist` (
    `id`                     INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`                   VARCHAR(32) NOT NULL DEFAULT '',
    `address`                VARCHAR(32) NOT NULL DEFAULT '127.0.0.1',
    `localAddress`           VARCHAR(255) NOT NULL DEFAULT '127.0.0.1',
    `localSubnetMask`        VARCHAR(255) NOT NULL DEFAULT '255.255.255.0',
    `port`                   INT NOT NULL DEFAULT 8085,
    `icon`                   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `realmflags`             TINYINT UNSIGNED NOT NULL DEFAULT 2,
    `timezone`               TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `allowedSecurityLevel`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `population`             FLOAT UNSIGNED NOT NULL DEFAULT 0,
    `realmbuilds`            VARCHAR(64)  NOT NULL DEFAULT '',
    PRIMARY KEY (`id`),
    UNIQUE KEY `idx_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `account_banned` (
    `id`         INT UNSIGNED NOT NULL DEFAULT 0,
    `bandate`    BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `unbandate`  BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `bannedby`   VARCHAR(50)  NOT NULL,
    `banreason`  VARCHAR(255) NOT NULL,
    `active`     TINYINT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (`id`, `bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `ip_banned` (
    `ip`        VARCHAR(32)  NOT NULL DEFAULT '127.0.0.1',
    `bandate`   BIGINT UNSIGNED NOT NULL,
    `unbandate` BIGINT UNSIGNED NOT NULL,
    `bannedby`  VARCHAR(50)  NOT NULL DEFAULT '[Console]',
    `banreason` VARCHAR(255) NOT NULL DEFAULT 'no reason',
    PRIMARY KEY (`ip`, `bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `realmcharacters` (
    `realmid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `acctid`   INT UNSIGNED NOT NULL DEFAULT 0,
    `numchars` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`realmid`, `acctid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Test accounts
INSERT IGNORE INTO `account` (`id`, `username`, `sha_pass_hash`, `gmlevel`, `expansion`) VALUES
    (1, 'TEST',  UPPER(SHA1(CONCAT(UPPER('TEST'),  ':', UPPER('TEST')))),  3, 3),
    (2, 'ADMIN', UPPER(SHA1(CONCAT(UPPER('ADMIN'), ':', UPPER('ADMIN')))), 3, 3);

INSERT IGNORE INTO `realmlist` (`id`, `name`, `address`, `port`, `realmbuilds`) VALUES
    (1, 'MaNGOS Three', '127.0.0.1', 8085, '15595');

-- ════════════════════════════════════════════════════════════════════════════
-- CHARACTER3
-- ════════════════════════════════════════════════════════════════════════════
USE `character3`;

CREATE TABLE IF NOT EXISTS `characters` (
    `guid`       INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `account`    INT UNSIGNED NOT NULL DEFAULT 0,
    `name`       VARCHAR(12)  NOT NULL DEFAULT '',
    `race`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `class`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `gender`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `level`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `xp`         INT UNSIGNED NOT NULL DEFAULT 0,
    `money`      BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `playerBytes`  INT UNSIGNED NOT NULL DEFAULT 0,
    `playerBytes2` INT UNSIGNED NOT NULL DEFAULT 0,
    `playerFlags`  INT UNSIGNED NOT NULL DEFAULT 0,
    `map`        INT UNSIGNED NOT NULL DEFAULT 0,
    `zone`       INT UNSIGNED NOT NULL DEFAULT 0,
    `position_x` FLOAT NOT NULL DEFAULT 0,
    `position_y` FLOAT NOT NULL DEFAULT 0,
    `position_z` FLOAT NOT NULL DEFAULT 0,
    `orientation` FLOAT NOT NULL DEFAULT 0,
    `health`     INT UNSIGNED NOT NULL DEFAULT 0,
    `power1`     INT UNSIGNED NOT NULL DEFAULT 0,
    `power2`     INT UNSIGNED NOT NULL DEFAULT 0,
    `power3`     INT UNSIGNED NOT NULL DEFAULT 0,
    `power4`     INT UNSIGNED NOT NULL DEFAULT 0,
    `power5`     INT UNSIGNED NOT NULL DEFAULT 0,
    `online`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `totaltime`  INT UNSIGNED NOT NULL DEFAULT 0,
    `leveltime`  INT UNSIGNED NOT NULL DEFAULT 0,
    `logout_time` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `is_logout_resting` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `rest_bonus` FLOAT NOT NULL DEFAULT 0,
    `at_login`   INT UNSIGNED NOT NULL DEFAULT 0,
    `deleteDate` INT UNSIGNED DEFAULT NULL,
    `deleteInfos_Account` INT UNSIGNED DEFAULT NULL,
    `deleteInfos_Name`    VARCHAR(12) DEFAULT NULL,
    `equipmentCache` LONGTEXT,
    `knownTitles`    LONGTEXT,
    `exploredZones`  LONGTEXT,
    PRIMARY KEY (`guid`),
    KEY `idx_account` (`account`),
    KEY `idx_online`  (`online`),
    KEY `idx_name`    (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_instance` (
    `guid`      INT UNSIGNED NOT NULL DEFAULT 0,
    `instance`  INT UNSIGNED NOT NULL DEFAULT 0,
    `map`       INT UNSIGNED NOT NULL DEFAULT 0,
    `difficulty` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `resettime` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `permanent` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `instance`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_aura` (
    `guid`       INT UNSIGNED NOT NULL DEFAULT 0,
    `caster_guid` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `item_guid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `spell`      INT UNSIGNED NOT NULL DEFAULT 0,
    `stackcount` INT UNSIGNED NOT NULL DEFAULT 1,
    `remaincharges` INT NOT NULL DEFAULT 0,
    `basepoints0`   INT NOT NULL DEFAULT 0,
    `basepoints1`   INT NOT NULL DEFAULT 0,
    `basepoints2`   INT NOT NULL DEFAULT 0,
    `periodictime0` INT UNSIGNED NOT NULL DEFAULT 0,
    `periodictime1` INT UNSIGNED NOT NULL DEFAULT 0,
    `periodictime2` INT UNSIGNED NOT NULL DEFAULT 0,
    `maxduration`   INT NOT NULL DEFAULT 0,
    `remaintime`    INT NOT NULL DEFAULT 0,
    `effIndexMask`  INT UNSIGNED NOT NULL DEFAULT 0,
    KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_spell` (
    `guid`     INT UNSIGNED NOT NULL DEFAULT 0,
    `spell`    INT UNSIGNED NOT NULL DEFAULT 0,
    `active`   TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `disabled` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `spell`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_action` (
    `guid`   INT UNSIGNED NOT NULL DEFAULT 0,
    `spec`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `button` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `action` INT UNSIGNED NOT NULL DEFAULT 0,
    `type`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `spec`, `button`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_homebind` (
    `guid`       INT UNSIGNED NOT NULL DEFAULT 0,
    `map`        INT UNSIGNED NOT NULL DEFAULT 0,
    `zone`       INT UNSIGNED NOT NULL DEFAULT 0,
    `position_x` FLOAT NOT NULL DEFAULT 0,
    `position_y` FLOAT NOT NULL DEFAULT 0,
    `position_z` FLOAT NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_inventory` (
    `guid` INT UNSIGNED NOT NULL DEFAULT 0,
    `bag`  INT UNSIGNED NOT NULL DEFAULT 0,
    `slot` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `item` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `bag`, `slot`),
    KEY `idx_item` (`item`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `item_instance` (
    `guid`       INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `owner_guid` INT UNSIGNED NOT NULL DEFAULT 0,
    `itemEntry`  INT UNSIGNED NOT NULL DEFAULT 0,
    `creatorGuid` INT UNSIGNED NOT NULL DEFAULT 0,
    `count`      INT UNSIGNED NOT NULL DEFAULT 1,
    `duration`   INT NOT NULL DEFAULT 0,
    `charges`    TINYTEXT,
    `flags`      INT UNSIGNED NOT NULL DEFAULT 0,
    `enchantments` TEXT,
    `randomPropertyId` INT NOT NULL DEFAULT 0,
    `durability` INT UNSIGNED NOT NULL DEFAULT 0,
    `text`       TEXT,
    PRIMARY KEY (`guid`),
    KEY `idx_owner_guid` (`owner_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_social` (
    `guid`   INT UNSIGNED NOT NULL DEFAULT 0,
    `friend` INT UNSIGNED NOT NULL DEFAULT 0,
    `flags`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `note`   VARCHAR(48) NOT NULL DEFAULT '',
    PRIMARY KEY (`guid`, `friend`, `flags`),
    KEY `friend_idx` (`friend`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_reputation` (
    `guid`    INT UNSIGNED NOT NULL DEFAULT 0,
    `faction` INT UNSIGNED NOT NULL DEFAULT 0,
    `standing` INT NOT NULL DEFAULT 0,
    `flags`   INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `faction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_queststatus` (
    `guid`       INT UNSIGNED NOT NULL DEFAULT 0,
    `quest`      INT UNSIGNED NOT NULL DEFAULT 0,
    `status`     INT UNSIGNED NOT NULL DEFAULT 0,
    `rewarded`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `explored`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `timer`      BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `mobcount1`  INT UNSIGNED NOT NULL DEFAULT 0,
    `mobcount2`  INT UNSIGNED NOT NULL DEFAULT 0,
    `mobcount3`  INT UNSIGNED NOT NULL DEFAULT 0,
    `mobcount4`  INT UNSIGNED NOT NULL DEFAULT 0,
    `itemcount1` INT UNSIGNED NOT NULL DEFAULT 0,
    `itemcount2` INT UNSIGNED NOT NULL DEFAULT 0,
    `itemcount3` INT UNSIGNED NOT NULL DEFAULT 0,
    `itemcount4` INT UNSIGNED NOT NULL DEFAULT 0,
    `itemcount5` INT UNSIGNED NOT NULL DEFAULT 0,
    `itemcount6` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `quest`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `guild` (
    `guildid`         INT UNSIGNED NOT NULL DEFAULT 0,
    `name`            VARCHAR(255) NOT NULL DEFAULT '',
    `leaderguid`      INT UNSIGNED NOT NULL DEFAULT 0,
    `EmblemStyle`     INT NOT NULL DEFAULT 0,
    `EmblemColor`     INT NOT NULL DEFAULT 0,
    `BorderStyle`     INT NOT NULL DEFAULT 0,
    `BorderColor`     INT NOT NULL DEFAULT 0,
    `BackgroundColor` INT NOT NULL DEFAULT 0,
    `info`            TEXT NOT NULL,
    `motd`            VARCHAR(128) NOT NULL DEFAULT '',
    `createdate`      BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `BankMoney`       BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guildid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `guild_member` (
    `guildid` INT UNSIGNED NOT NULL DEFAULT 0,
    `guid`    INT UNSIGNED NOT NULL DEFAULT 0,
    `rank`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `pnote`   VARCHAR(31) NOT NULL DEFAULT '',
    `offnote` VARCHAR(31) NOT NULL DEFAULT '',
    PRIMARY KEY (`guildid`, `guid`),
    UNIQUE KEY `guid_key` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_pet` (
    `id`       INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `entry`    INT UNSIGNED NOT NULL DEFAULT 0,
    `owner`    INT UNSIGNED NOT NULL DEFAULT 0,
    `modelid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `level`    INT UNSIGNED NOT NULL DEFAULT 1,
    `exp`      INT UNSIGNED NOT NULL DEFAULT 0,
    `Reactstate` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `slot`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `name`     VARCHAR(21) NOT NULL DEFAULT 'Pet',
    `renamed`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `curhealth` INT UNSIGNED NOT NULL DEFAULT 1,
    `curmana`  INT UNSIGNED NOT NULL DEFAULT 0,
    `abdata`   TEXT,
    `savetime` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `resettalents_cost` INT UNSIGNED NOT NULL DEFAULT 0,
    `resettalents_time` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `CreatedBySpell` INT UNSIGNED NOT NULL DEFAULT 0,
    `PetType`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`),
    KEY `idx_owner` (`owner`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_declinedname` (
    `guid`          INT UNSIGNED NOT NULL DEFAULT 0,
    `genitive`      VARCHAR(15) NOT NULL DEFAULT '',
    `dative`        VARCHAR(15) NOT NULL DEFAULT '',
    `accusative`    VARCHAR(15) NOT NULL DEFAULT '',
    `instrumental`  VARCHAR(15) NOT NULL DEFAULT '',
    `prepositional` VARCHAR(15) NOT NULL DEFAULT '',
    PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_queststatus_daily` (
    `guid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `quest` INT UNSIGNED NOT NULL DEFAULT 0,
    `time`  BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `quest`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_queststatus_weekly` (
    `guid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `quest` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `quest`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_queststatus_monthly` (
    `guid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `quest` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `quest`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_spell_cooldown` (
    `guid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `spell` INT UNSIGNED NOT NULL DEFAULT 0,
    `item`  INT UNSIGNED NOT NULL DEFAULT 0,
    `time`  BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `spell`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_account_data` (
    `guid` INT UNSIGNED NOT NULL DEFAULT 0,
    `type` INT UNSIGNED NOT NULL DEFAULT 0,
    `time` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `data` LONGBLOB,
    PRIMARY KEY (`guid`, `type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_talent` (
    `guid`         INT UNSIGNED NOT NULL DEFAULT 0,
    `talent_id`    INT UNSIGNED NOT NULL DEFAULT 0,
    `current_rank` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `spec`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `talent_id`, `spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_skills` (
    `guid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `skill` INT UNSIGNED NOT NULL DEFAULT 0,
    `value` INT UNSIGNED NOT NULL DEFAULT 0,
    `max`   INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `skill`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_glyphs` (
    `guid`  INT UNSIGNED NOT NULL DEFAULT 0,
    `spec`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `slot`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `glyph` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `spec`, `slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_achievement` (
    `guid`        INT UNSIGNED NOT NULL DEFAULT 0,
    `achievement` INT UNSIGNED NOT NULL DEFAULT 0,
    `date`        BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `achievement`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_achievement_progress` (
    `guid`    INT UNSIGNED NOT NULL DEFAULT 0,
    `criteria` INT UNSIGNED NOT NULL DEFAULT 0,
    `counter` INT UNSIGNED NOT NULL DEFAULT 0,
    `date`    BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `criteria`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_equipmentsets` (
    `guid`        INT UNSIGNED NOT NULL DEFAULT 0,
    `setguid`     BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `setindex`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `name`        VARCHAR(31) NOT NULL DEFAULT '',
    `iconname`    VARCHAR(100) NOT NULL DEFAULT '',
    `ignore_mask` INT UNSIGNED NOT NULL DEFAULT 0,
    `item0`  INT UNSIGNED NOT NULL DEFAULT 0, `item1`  INT UNSIGNED NOT NULL DEFAULT 0,
    `item2`  INT UNSIGNED NOT NULL DEFAULT 0, `item3`  INT UNSIGNED NOT NULL DEFAULT 0,
    `item4`  INT UNSIGNED NOT NULL DEFAULT 0, `item5`  INT UNSIGNED NOT NULL DEFAULT 0,
    `item6`  INT UNSIGNED NOT NULL DEFAULT 0, `item7`  INT UNSIGNED NOT NULL DEFAULT 0,
    `item8`  INT UNSIGNED NOT NULL DEFAULT 0, `item9`  INT UNSIGNED NOT NULL DEFAULT 0,
    `item10` INT UNSIGNED NOT NULL DEFAULT 0, `item11` INT UNSIGNED NOT NULL DEFAULT 0,
    `item12` INT UNSIGNED NOT NULL DEFAULT 0, `item13` INT UNSIGNED NOT NULL DEFAULT 0,
    `item14` INT UNSIGNED NOT NULL DEFAULT 0, `item15` INT UNSIGNED NOT NULL DEFAULT 0,
    `item16` INT UNSIGNED NOT NULL DEFAULT 0, `item17` INT UNSIGNED NOT NULL DEFAULT 0,
    `item18` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`setguid`),
    UNIQUE KEY `idx_set` (`guid`, `setguid`, `setindex`),
    KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_currencies` (
    `guid`         INT UNSIGNED NOT NULL DEFAULT 0,
    `id`           INT UNSIGNED NOT NULL DEFAULT 0,
    `totalCount`   INT UNSIGNED NOT NULL DEFAULT 0,
    `weekCount`    INT UNSIGNED NOT NULL DEFAULT 0,
    `seasonCount`  INT UNSIGNED NOT NULL DEFAULT 0,
    `flags`        INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_battleground_data` (
    `guid`        INT UNSIGNED NOT NULL DEFAULT 0,
    `instance_id` INT UNSIGNED NOT NULL DEFAULT 0,
    `team`        INT UNSIGNED NOT NULL DEFAULT 0,
    `join_x`      FLOAT NOT NULL DEFAULT 0,
    `join_y`      FLOAT NOT NULL DEFAULT 0,
    `join_z`      FLOAT NOT NULL DEFAULT 0,
    `join_o`      FLOAT NOT NULL DEFAULT 0,
    `join_map`    INT UNSIGNED NOT NULL DEFAULT 0,
    `taxi_start`  INT UNSIGNED NOT NULL DEFAULT 0,
    `taxi_end`    INT UNSIGNED NOT NULL DEFAULT 0,
    `mount_spell` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `group_member` (
    `guid`      INT UNSIGNED NOT NULL DEFAULT 0,
    `memberGuid` INT UNSIGNED NOT NULL DEFAULT 0,
    `groupId`   INT UNSIGNED NOT NULL DEFAULT 0,
    `assistant`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `subgroup`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`memberGuid`),
    KEY `idx_group` (`groupId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `instance` (
    `id`        INT UNSIGNED NOT NULL DEFAULT 0,
    `map`       INT UNSIGNED NOT NULL DEFAULT 0,
    `resettime` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `difficulty` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `data`      LONGTEXT,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `item_loot` (
    `guid`       INT UNSIGNED NOT NULL DEFAULT 0,
    `owner_guid` INT UNSIGNED NOT NULL DEFAULT 0,
    `itemid`     INT UNSIGNED NOT NULL DEFAULT 0,
    `amount`     INT UNSIGNED NOT NULL DEFAULT 0,
    `suffix`     INT NOT NULL DEFAULT 0,
    `property`   INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `itemid`),
    KEY `idx_owner` (`owner_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `mail` (
    `id`             INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `messageType`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `stationery`     TINYINT NOT NULL DEFAULT 41,
    `mailTemplateId` INT UNSIGNED NOT NULL DEFAULT 0,
    `sender`         INT UNSIGNED NOT NULL DEFAULT 0,
    `receiver`       INT UNSIGNED NOT NULL DEFAULT 0,
    `subject`        VARCHAR(200) DEFAULT '',
    `body`           VARCHAR(500) DEFAULT '',
    `has_items`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `expire_time`    BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `deliver_time`   BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `money`          BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `cod`            BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `checked`        TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`),
    KEY `idx_receiver` (`receiver`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `mail_items` (
    `mail_id`       INT UNSIGNED NOT NULL DEFAULT 0,
    `item_guid`     INT UNSIGNED NOT NULL DEFAULT 0,
    `item_template` INT UNSIGNED NOT NULL DEFAULT 0,
    `receiver`      INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`mail_id`, `item_guid`),
    KEY `idx_receiver` (`receiver`),
    KEY `idx_item_guid` (`item_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `arena_team_member` (
    `arenateamid`    INT UNSIGNED NOT NULL DEFAULT 0,
    `guid`           INT UNSIGNED NOT NULL DEFAULT 0,
    `played_week`    INT UNSIGNED NOT NULL DEFAULT 0,
    `wons_week`      INT UNSIGNED NOT NULL DEFAULT 0,
    `played_season`  INT UNSIGNED NOT NULL DEFAULT 0,
    `wons_season`    INT UNSIGNED NOT NULL DEFAULT 0,
    `personal_rating` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`arenateamid`, `guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ════════════════════════════════════════════════════════════════════════════
-- MANGOS3 (World)
-- ════════════════════════════════════════════════════════════════════════════
USE `mangos3`;

CREATE TABLE IF NOT EXISTS `db_version` (
    `version`             VARCHAR(120) DEFAULT NULL,
    `creature_ai_version` VARCHAR(120) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `creature_template` (
    `entry`          INT UNSIGNED NOT NULL DEFAULT 0,
    `Name`           VARCHAR(100) NOT NULL DEFAULT '',
    `SubName`        VARCHAR(100) DEFAULT '',
    `ModelId1`       INT UNSIGNED NOT NULL DEFAULT 0,
    `ModelId2`       INT UNSIGNED NOT NULL DEFAULT 0,
    `ModelId3`       INT UNSIGNED NOT NULL DEFAULT 0,
    `ModelId4`       INT UNSIGNED NOT NULL DEFAULT 0,
    `MinLevel`       TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `MaxLevel`       TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `MinLevelHealth` INT UNSIGNED NOT NULL DEFAULT 0,
    `MaxLevelHealth` INT UNSIGNED NOT NULL DEFAULT 0,
    `MinLevelMana`   INT UNSIGNED NOT NULL DEFAULT 0,
    `MaxLevelMana`   INT UNSIGNED NOT NULL DEFAULT 0,
    `MinMeleeDmg`    FLOAT NOT NULL DEFAULT 0,
    `MaxMeleeDmg`    FLOAT NOT NULL DEFAULT 0,
    `Armor`          INT UNSIGNED NOT NULL DEFAULT 0,
    `faction`        INT UNSIGNED NOT NULL DEFAULT 0,
    `UnitFlags`      INT UNSIGNED NOT NULL DEFAULT 0,
    `UnitClass`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `Rank`           TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `creature` (
    `guid`           INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `id`             INT UNSIGNED NOT NULL DEFAULT 0,
    `map`            INT UNSIGNED NOT NULL DEFAULT 0,
    `position_x`     FLOAT NOT NULL DEFAULT 0,
    `position_y`     FLOAT NOT NULL DEFAULT 0,
    `position_z`     FLOAT NOT NULL DEFAULT 0,
    `orientation`    FLOAT NOT NULL DEFAULT 0,
    `spawntimesecsmin` INT UNSIGNED NOT NULL DEFAULT 120,
    `spawntimesecsmax` INT UNSIGNED NOT NULL DEFAULT 120,
    PRIMARY KEY (`guid`),
    KEY `idx_id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ── Done ─────────────────────────────────────────────────────────────────────
SELECT 'MaNGOS MariaDB setup complete' AS status;
