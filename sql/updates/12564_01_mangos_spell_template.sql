ALTER TABLE db_version CHANGE COLUMN required_c12552_01_mangos_item_enchantment_template required_c12564_01_mangos_spell_template bit;

DELETE FROM spell_template WHERE id IN (37264, 37278, 37365);
INSERT INTO spell_template (`id`,`proc_flags`,`proc_chance`,`duration_index`,`effect0`,`effect0_implicit_target_a`,`effect0_radius_idx`,
`effect0_apply_aura_name`,`effect0_misc_value`,`effect0_misc_value_b`,`effect0_trigger_spell`,`comments`) VALUES
(37264, 0x00000000, 101, 21, 28, 18, 7, 0, 21729, 64, 0, 'Power Converters: Summon Electromental (from cata)'),
(37278, 0x00000000, 101, 21, 28, 18, 1, 0, 21737, 64, 0, 'Power Converters: Summon Mini-Electromental (from cata)'),
(37365, 0x00000000, 101, 21, 28, 18, 1, 0, 21757, 64, 0, 'Power Converters: Summon Big Flavor Electromental (from cata)');
