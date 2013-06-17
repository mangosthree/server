ALTER TABLE db_version CHANGE COLUMN required_c12631_01_mangos_creature required_c12631_02_mangos_gameobject bit;

ALTER TABLE gameobject MODIFY COLUMN `phaseMask` int(11) unsigned NOT NULL DEFAULT '1';
