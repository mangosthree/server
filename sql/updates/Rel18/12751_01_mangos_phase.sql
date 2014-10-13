ALTER TABLE db_version CHANGE COLUMN required_12741_01_mangos required_12751_01_mangos_phase bit;

DROP TABLE IF EXISTS `phase_definitions`;
CREATE TABLE `phase_definitions` (
  `zoneId` mediumint(7) unsigned NOT NULL DEFAULT '0',
  `entry` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `phasemask` bigint(20) unsigned NOT NULL DEFAULT '0',
  `phaseId` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `terrainswapmap` smallint(5) unsigned NOT NULL DEFAULT '0',
  `flags` tinyint(3) unsigned DEFAULT '0',
  `condition_id` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `comment` text,
  PRIMARY KEY (`zoneId`,`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `spell_phase`;
CREATE TABLE `spell_phase` (
  `id` mediumint(7) unsigned NOT NULL DEFAULT '0',
  `phasemask` bigint(20) unsigned NOT NULL DEFAULT '1',
  `terrainswapmap` smallint(5) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `quest_phase_maps`;
