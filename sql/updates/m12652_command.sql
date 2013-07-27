update command set `name`='summon' where `name`='namego';
update command set `name`='appear' where `name`='goname';

ALTER TABLE db_version CHANGE COLUMN required_c12631_01_mangos_creature required_m12652_command bit;