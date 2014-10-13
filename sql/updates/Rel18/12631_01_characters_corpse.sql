ALTER TABLE character_db_version CHANGE COLUMN required_12447_02_characters_calendar_invites required_12631_01_characters_corpse bit;

ALTER TABLE corpse MODIFY COLUMN `phaseMask` int(11) unsigned NOT NULL DEFAULT '1';