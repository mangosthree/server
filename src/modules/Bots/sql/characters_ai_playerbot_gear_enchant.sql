DROP TABLE IF EXISTS `ai_playerbot_gear_enchant`;
CREATE TABLE `ai_playerbot_gear_enchant` (
  `class` tinyint(3) unsigned NOT NULL,
  `spec` varchar(24) NOT NULL,
  `slot` varchar(12) NOT NULL,
  `enchant` int(10) unsigned NOT NULL,
  PRIMARY KEY (`class`,`spec`,`slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Curated permanent enchant (SpellItemEnchantment id) per class/spec/slot for playerbots (Phase B: enchants)';
