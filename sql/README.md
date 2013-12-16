-- FOLDERS:
updates - contains updates for characters, mangos and realmd databases.

tools - contains additional useful queries.

-- SQL QUERIES:
characters.sql - creates database in MySQL/PostgreSQL, which holds important every player character data.

create_mysql.sql - creates initial databases structure for MaNGOS server in MySQL server. 
This query will create three databases: "characters" (its characters database), "mangos" (its world database) and "realmd" (its accounts database)
 databases. Also will created new MySQL user - "mangos", which is default in MaNGOS server.

drop_mysql.sql - drops "characters", "realmd", "mangos" databases and user, created by create_mysql.sql query (if exists in MySQL server).

mangos.sql - creates empty world database. Mostly used by developers, to coordinate basic world database structure.

mangos_spell_check.sql - contain expected by code spell properties from simple existence to spell family mask and icons.
This data let easy detect spell properties change at client version support switch and update or drop problematic code associated with checked 
spell data. Sql file not required for server work but expected to be applied to MaNGOS DB if you plan use .debug spellcheck command in console.           */

postgre_compatibility_addon.sql - contains queries for PostgreSQL users. Its need apply single time after DB creating or later (safe reapply) and let 
correctly execute SQL queries with UNIX_TIMESTAMP/FROM_UNIXTIME functions uses in mangos/realmd code.

realmd.sql - creates database in MySQL/PostgreSQL, which holds player game account information.
NOTE: for testing purposes, this sql query contains four accounts data by default:
        * username: ADMINISTRATOR password: ADMINISTRATOR, security level 3, has almost all administrator features.
        * username: GAMEMASTER password: GAMEMASTER, security level is 2, has limited access.
        * username: MODERATOR password: MODERATOR, security level is 1, has moderator access features (chat, tickets...).
        * username: PLAYER password: PLAYER, security level is 0, its simple player account, has no access to gm commands.
If still want to use clear realmd database, make sure, that on these accounts there wasn't created any characters, if is, truncate or reimport
characters.sql and execute all these queries on realmd database - "truncate table account; truncate table realmcharacters;" (without quates).
