ALTER TABLE db_version CHANGE COLUMN required_c12631_02_mangos_gameobject required_m12654_command bit;
update command set `name`='summon' where `name`='namego';
update command set `name`='appear' where `name`='goname';