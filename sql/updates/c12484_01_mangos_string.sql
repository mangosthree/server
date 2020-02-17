ALTER TABLE db_version CHANGE COLUMN required_c12472_02_mangos_npc_vendor_template required_c12484_01_mangos_string bit;

DELETE FROM `mangos_string` WHERE `entry` = 11001;