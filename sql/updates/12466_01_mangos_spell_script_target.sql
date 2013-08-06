ALTER TABLE db_version CHANGE COLUMN required_12458_01_mangos_dbscripts_on_creature_death required_c12466_01_mangos_spell_script_target bit;

ALTER TABLE spell_script_target ADD COLUMN `inverseEffectMask` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `targetEntry`;
