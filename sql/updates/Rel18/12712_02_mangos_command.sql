ALTER TABLE db_version CHANGE COLUMN required_12712_01_mangos required_12712_02_mangos_command bit;

UPDATE `command` SET `help`='Syntax: .account set addon [#accountId|$accountName] #addon\r\n\r\nSet user (possible targeted) expansion addon level allowed. Addon values: 0 - normal, 1 - tbc, 2 - wotlk, 3 - cataclysm.' WHERE  `name`='account set addon';
