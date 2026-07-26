DROP TABLE IF EXISTS `ai_playerbot_glyph`;
CREATE TABLE `ai_playerbot_glyph` (
  `class` tinyint(3) unsigned NOT NULL,
  `spec` varchar(24) NOT NULL,
  `glyph_type` varchar(6) NOT NULL,
  `glyph_spell` int(10) unsigned NOT NULL,
  `slot_idx` tinyint(3) unsigned NOT NULL,
  PRIMARY KEY (`class`,`spec`,`glyph_type`,`slot_idx`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Curated glyph (prime/major/minor) per class/spec for playerbots (Phase D: glyphs); glyph_spell is the real glyph-apply spell (item spellid_2)';
