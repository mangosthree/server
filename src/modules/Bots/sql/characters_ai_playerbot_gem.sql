DROP TABLE IF EXISTS `ai_playerbot_gem`;
CREATE TABLE `ai_playerbot_gem` (
  `class` tinyint(3) unsigned NOT NULL,
  `spec` varchar(24) NOT NULL,
  `color` varchar(10) NOT NULL,
  `gem_item` int(10) unsigned NOT NULL,
  `gem_enchant` int(10) unsigned NOT NULL,
  PRIMARY KEY (`class`,`spec`,`color`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Curated socket gem (color -> gem item + SpellItemEnchantment id) per class/spec for playerbots (Phase C: gems)';
