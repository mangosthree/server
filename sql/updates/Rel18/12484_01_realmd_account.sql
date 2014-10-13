ALTER TABLE realmd_db_version CHANGE COLUMN required_12112_01_realmd_account_access required_c12484_01_realmd_account bit;

ALTER TABLE account ADD COLUMN `gmlevel` tinyint(3) unsigned NOT NULL DEFAULT '0' AFTER `sha_pass_hash`;
UPDATE account,account_access SET account.gmlevel = account_access.gmlevel WHERE account.id=account_access.id AND account_access.RealmID = -1;