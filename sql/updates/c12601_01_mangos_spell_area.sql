ALTER TABLE db_version CHANGE COLUMN required_c12594_01_mangos_spell_template required_c12601_01_mangos_spell_area bit;

ALTER TABLE spell_area ADD COLUMN `condition_id` mediumint(8) unsigned NOT NULL default '0' AFTER quest_end;
