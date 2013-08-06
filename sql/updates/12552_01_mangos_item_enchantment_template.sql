ALTER TABLE db_version CHANGE COLUMN required_c12484_01_mangos_string required_c12552_01_mangos_item_enchantment_template bit;

ALTER TABLE item_enchantment_template MODIFY COLUMN entry mediumint(8) NOT NULL DEFAULT '0';