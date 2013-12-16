<b>-- FOLDERS:</b>

updates - contains updates for characters, mangos and realmd databases.

tools - contains additional useful queries.

<b>-- SQL QUERIES:</b>


<b>characters.sql</b> - creates database in MySQL/PostgreSQL, which holds important every player character data.


<b>create_mysql.sql</b> - creates initial databases structure for MaNGOS server in MySQL server. 
This query will create three databases: "characters" (its characters database), "mangos" (its world database) and "realmd" (its accounts database)
 databases. Also will created new MySQL user - "mangos", which is default in MaNGOS server.


<b>drop_mysql.sql</b> - drops "characters", "realmd", "mangos" databases and user, created by create_mysql.sql query (if exists in MySQL server).


<b>mangos.sql</b> - creates empty world database. Mostly used by developers, to coordinate basic world database structure.


<b>mangos_spell_check.sql</b> - contain expected by code spell properties from simple existence to spell family mask and icons.
This data let easy detect spell properties change at client version support switch and update or drop problematic code associated with checked 
spell data. Sql file not required for server work but expected to be applied to MaNGOS DB if you plan use .debug spellcheck command in console.           */


<b>postgre_compatibility_addon.sql</b> - contains queries for PostgreSQL users. Its need apply single time after DB creating or later (safe reapply) and let 
correctly execute SQL queries with UNIX_TIMESTAMP/FROM_UNIXTIME functions uses in mangos/realmd code.


<b>realmd.sql</b> - creates database in MySQL/PostgreSQL, which holds player game account information.
NOTE: for testing purposes, this sql query contains four accounts data by default:

<table>
 <tr>
  <th>username</th>
  <th>password</th>
  <th>security level</th>
  <th>comment</th>
 </tr>
 <tr>
  <td>ADMINISTRATOR</td>
  <td>ADMINISTRATOR</td>
  <td>3</td>
  <td>has almost all administrator features</td>
 </tr>
 <tr>
  <td>GAMEMASTER</td>
  <td>GAMEMASTER</td>
  <td>2</td>
  <td>has limited access</td>
 </tr>
 <tr>
  <td>MODERATOR</td>
  <td>MODERATOR</td>
  <td>1</td>
  <td>has moderator access features (chat, tickets...)</td>
 </tr>
 <tr>
  <td>PLAYER</td>
  <td>PLAYER</td>
  <td>0</td>
  <td>its simple player account, has no access to gm commands</td>
 </tr>
</table>

If still want to use clear realmd database, make sure, that on these accounts there wasn't created any characters, if is, truncate or reimport characters.sql and execute all these queries on realmd database - "truncate table account; truncate table realmcharacters;" (without quates).
