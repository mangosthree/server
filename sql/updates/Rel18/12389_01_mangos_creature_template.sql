ALTER TABLE db_version CHANGE COLUMN required_12388_01_mangos_creature_template required_12389_01_mangos_creature_template bit;

UPDATE creature_template SET flags_extra=flags_extra & ~0x800;
