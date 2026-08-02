DROP TABLE IF EXISTS `ai_playerbot_gear`;
CREATE TABLE `ai_playerbot_gear` (
  `class` tinyint(3) unsigned NOT NULL,
  `spec` varchar(24) NOT NULL,
  `tier` varchar(8) NOT NULL,
  `slot` varchar(12) NOT NULL,
  `item` int(10) unsigned NOT NULL,
  PRIMARY KEY (`class`,`spec`,`tier`,`slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Curated best-in-slot gear per class/spec/tier for playerbots (Phase A: gear only)';
