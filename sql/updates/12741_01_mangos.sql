ALTER TABLE db_version CHANGE COLUMN required_12738_01_mangos_spell_template required_12741_01_mangos bit;

update creature_ai_scripts set action1_type=48 where action1_type=47;
update creature_ai_scripts set action2_type=48 where action2_type=47;
update creature_ai_scripts set action3_type=48 where action3_type=47;
