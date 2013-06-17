ALTER TABLE db_version CHANGE COLUMN required_c12602_01_mangos_npc_spellclick_spells required_c12631_01_mangos_creature bit;

ALTER TABLE creature MODIFY COLUMN `phaseMask` int(11) unsigned NOT NULL DEFAULT '1';
