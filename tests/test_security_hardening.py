#!/usr/bin/env python3
"""
MaNGOS Three — Security Hardening Validation Suite

Tests that all security fixes applied in security_hardening.sql are working:
- Privilege reduction (mangos user cannot DROP/ALTER/CREATE)
- CHECK constraints (money cap, level range, item count)
- Foreign key integrity (no orphaned records)
- Account lockout tracking
- SQL injection resistance via parameterized queries
- Audit log table existence
- Rate limiting table existence
- Normal game actions work correctly after hardening

Requires: pip install mysql-connector-python
"""

import sys
import os
import time
import threading
import random

try:
    import mysql.connector as db_driver
except ImportError:
    print("ERROR: pip install mysql-connector-python")
    sys.exit(1)

DB_HOST = "127.0.0.1"
DB_PORT = 3306
DB_USER = os.environ.get("DB_USER", "mangos")
DB_PASS = os.environ.get("DB_PASS", "mangos")

RESULTS = {"passed": 0, "failed": 0, "errors": []}

# ── helpers ──────────────────────────────────────────────────────────────────

def conn(db, autocommit=True):
    return db_driver.connect(
        host=DB_HOST, port=DB_PORT, user=DB_USER, password=DB_PASS,
        database=db, autocommit=autocommit, connection_timeout=5
    )

def root_conn(db, autocommit=True):
    return db_driver.connect(
        host=DB_HOST, port=DB_PORT, user="root", password="",
        database=db, autocommit=autocommit, connection_timeout=5
    )

def record(name, passed, note=""):
    tag = "PASS" if passed else "FAIL"
    RESULTS["passed" if passed else "failed"] += 1
    if not passed:
        RESULTS["errors"].append(f"{name}: {note}")
    print(f"  [{tag}] {name}" + (f" — {note}" if note else ""))

def run_test(name):
    def decorator(func):
        def wrapper():
            try:
                func()
                record(name, True)
            except AssertionError as e:
                record(name, False, str(e))
            except Exception as e:
                record(name, False, f"{type(e).__name__}: {e}")
        wrapper.__name__ = name
        wrapper._test = True
        return wrapper
    return decorator


# ═══════════════════════════════════════════════════════════════════════════
# 1. PRIVILEGE REDUCTION — mangos user cannot do DDL
# ═══════════════════════════════════════════════════════════════════════════

@run_test("Privilege_MangosCannotDropTable")
def test_priv_no_drop():
    """mangos user must not be able to DROP any table."""
    c = conn("realmd")
    cur = c.cursor()
    try:
        cur.execute("DROP TABLE IF EXISTS account_banned")
        # If we get here, DROP succeeded — that's a privilege failure
        # Restore the table if somehow dropped (shouldn't happen)
        assert False, "mangos user was able to DROP a table — privilege not restricted"
    except db_driver.errors.ProgrammingError as e:
        # Expected: access denied
        assert "denied" in str(e).lower() or "access" in str(e).lower(), \
            f"Unexpected error: {e}"
    finally:
        cur.close()
        c.close()

@run_test("Privilege_MangosCannotCreateTable")
def test_priv_no_create():
    """mangos user must not be able to CREATE new tables."""
    c = conn("realmd")
    cur = c.cursor()
    try:
        cur.execute("CREATE TABLE exploit_table (id INT PRIMARY KEY)")
        cur.execute("DROP TABLE IF EXISTS exploit_table")
        assert False, "mangos user was able to CREATE a table — privilege not restricted"
    except db_driver.errors.ProgrammingError as e:
        assert "denied" in str(e).lower() or "access" in str(e).lower(), \
            f"Unexpected error: {e}"
    finally:
        cur.close()
        c.close()

@run_test("Privilege_MangosCannotAlterTable")
def test_priv_no_alter():
    """mangos user must not be able to ALTER tables."""
    c = conn("realmd")
    cur = c.cursor()
    try:
        cur.execute("ALTER TABLE account ADD COLUMN exploit_col VARCHAR(10)")
        cur.execute("ALTER TABLE account DROP COLUMN exploit_col")
        assert False, "mangos user was able to ALTER a table — privilege not restricted"
    except db_driver.errors.ProgrammingError as e:
        assert "denied" in str(e).lower() or "access" in str(e).lower(), \
            f"Unexpected error: {e}"
    finally:
        cur.close()
        c.close()

@run_test("Privilege_MangosCannotGrant")
def test_priv_no_grant():
    """mangos user must not be able to GRANT privileges."""
    c = conn("realmd")
    cur = c.cursor()
    try:
        cur.execute("GRANT ALL ON realmd.* TO 'hacker'@'localhost'")
        assert False, "mangos user was able to GRANT privileges — critical vulnerability"
    except (db_driver.errors.ProgrammingError, db_driver.errors.DatabaseError):
        pass  # expected
    finally:
        cur.close()
        c.close()

@run_test("Privilege_MangosCannotTruncate")
def test_priv_no_truncate():
    """mangos user must not be able to TRUNCATE the account table."""
    c = conn("realmd")
    cur = c.cursor()
    try:
        cur.execute("TRUNCATE TABLE account")
        # If this runs, check account still has data
        cur.execute("SELECT COUNT(*) FROM account")
        count = cur.fetchone()[0]
        if count == 0:
            assert False, "mangos user TRUNCATED the account table — data lost!"
        # TRUNCATE succeeded but data still exists (MariaDB might allow it with SUPER)
        # Re-insert test accounts
    except (db_driver.errors.ProgrammingError, db_driver.errors.DatabaseError):
        pass  # expected — TRUNCATE requires DROP privilege
    finally:
        cur.close()
        c.close()

@run_test("Privilege_MangosCanReadData")
def test_priv_can_read():
    """mangos user must still be able to SELECT data (regression check)."""
    c = conn("realmd")
    cur = c.cursor()
    cur.execute("SELECT id, username FROM account WHERE username = 'TEST'")
    row = cur.fetchone()
    assert row is not None, "TEST account not found — read privileges broken"
    assert row[1] == "TEST", f"Unexpected username: {row[1]}"
    cur.close()
    c.close()

@run_test("Privilege_MangosCanWriteData")
def test_priv_can_write():
    """mangos user must still be able to INSERT/UPDATE/DELETE (regression check)."""
    c = conn("character3")
    cur = c.cursor()
    # Insert a test character
    cur.execute(
        "INSERT INTO characters (guid, account, name, race, class, level, xp, money) "
        "VALUES (999990, 1, 'PrivTest', 1, 1, 1, 0, 100)"
    )
    cur.execute("SELECT guid FROM characters WHERE guid = 999990")
    assert cur.fetchone() is not None, "INSERT failed"
    cur.execute("UPDATE characters SET level = 2 WHERE guid = 999990")
    cur.execute("SELECT level FROM characters WHERE guid = 999990")
    assert cur.fetchone()[0] == 2, "UPDATE failed"
    cur.execute("DELETE FROM characters WHERE guid = 999990")
    cur.execute("SELECT guid FROM characters WHERE guid = 999990")
    assert cur.fetchone() is None, "DELETE failed"
    cur.close()
    c.close()


# ═══════════════════════════════════════════════════════════════════════════
# 2. CHECK CONSTRAINTS — data integrity enforcement
# ═══════════════════════════════════════════════════════════════════════════

@run_test("Constraint_MoneyCapEnforced")
def test_money_cap():
    """Characters cannot have more than 9,999,999,999 money."""
    c = conn("character3")
    cur = c.cursor()
    try:
        cur.execute(
            "INSERT INTO characters (guid, account, name, race, class, level, money) "
            "VALUES (999991, 1, 'MoneyHack', 1, 1, 1, 99999999999)"  # over cap
        )
        cur.execute("DELETE FROM characters WHERE guid = 999991")
        # If we get here, the check constraint wasn't enforced
        assert False, "Money cap CHECK constraint not enforced — gold exploit possible"
    except (db_driver.errors.DatabaseError, db_driver.errors.IntegrityError) as e:
        # Expected: constraint violation
        pass
    finally:
        cur.execute("DELETE FROM characters WHERE guid = 999991")
        cur.close()
        c.close()

@run_test("Constraint_MoneyCapAllowsMax")
def test_money_cap_max_allowed():
    """Exactly 9,999,999,999 money must be allowed."""
    c = conn("character3")
    cur = c.cursor()
    cur.execute(
        "INSERT INTO characters (guid, account, name, race, class, level, money) "
        "VALUES (999992, 1, 'MaxMoney', 1, 1, 1, 9999999999)"
    )
    cur.execute("SELECT money FROM characters WHERE guid = 999992")
    assert cur.fetchone()[0] == 9999999999, "Max money value not stored correctly"
    cur.execute("DELETE FROM characters WHERE guid = 999992")
    cur.close()
    c.close()

@run_test("Constraint_LevelRangeEnforced")
def test_level_range():
    """Character level must be between 0 and 85."""
    c = conn("character3")
    cur = c.cursor()
    try:
        cur.execute(
            "INSERT INTO characters (guid, account, name, race, class, level) "
            "VALUES (999993, 1, 'LevelHack', 1, 1, 255)"  # over max
        )
        cur.execute("DELETE FROM characters WHERE guid = 999993")
        assert False, "Level range CHECK constraint not enforced — level hack possible"
    except (db_driver.errors.DatabaseError, db_driver.errors.IntegrityError):
        pass
    finally:
        cur.execute("DELETE FROM characters WHERE guid = 999993")
        cur.close()
        c.close()

@run_test("Constraint_ItemCountPositive")
def test_item_count():
    """Item count must be positive."""
    c = conn("character3")
    cur = c.cursor()
    try:
        cur.execute(
            "INSERT INTO item_instance (guid, owner_guid, itemEntry, count) "
            "VALUES (999999, 1, 25, 0)"  # zero count
        )
        cur.execute("DELETE FROM item_instance WHERE guid = 999999")
        assert False, "Item count CHECK constraint not enforced — item dupe exploit possible"
    except (db_driver.errors.DatabaseError, db_driver.errors.IntegrityError):
        pass
    finally:
        cur.execute("DELETE FROM item_instance WHERE guid = 999999")
        cur.close()
        c.close()

@run_test("Constraint_MailMoneyCapEnforced")
def test_mail_money_cap():
    """Mail money cannot exceed 9,999,999,999."""
    c = conn("character3")
    cur = c.cursor()
    try:
        cur.execute(
            "INSERT INTO mail (id, messageType, stationery, sender, receiver, money, "
            "expire_time, deliver_time) "
            "VALUES (999999, 0, 41, 1, 2, 99999999999, 0, 0)"
        )
        cur.execute("DELETE FROM mail WHERE id = 999999")
        assert False, "Mail money cap CHECK constraint not enforced — gold dupe via mail possible"
    except (db_driver.errors.DatabaseError, db_driver.errors.IntegrityError):
        pass
    finally:
        cur.execute("DELETE FROM mail WHERE id = 999999")
        cur.close()
        c.close()


# ═══════════════════════════════════════════════════════════════════════════
# 3. FOREIGN KEY INTEGRITY
# ═══════════════════════════════════════════════════════════════════════════

@run_test("FK_OrphanedInventoryPrevented")
def test_fk_inventory():
    """Cannot insert inventory for a non-existent character."""
    c = conn("character3")
    cur = c.cursor()
    try:
        cur.execute(
            "INSERT INTO character_inventory (guid, bag, slot, item) "
            "VALUES (88888888, 0, 0, 1)"  # guid 88888888 doesn't exist
        )
        cur.execute("DELETE FROM character_inventory WHERE guid = 88888888")
        assert False, "FK constraint on character_inventory not enforced — orphaned items possible"
    except (db_driver.errors.DatabaseError, db_driver.errors.IntegrityError) as e:
        # Expected: FK violation
        pass
    finally:
        cur.execute("DELETE FROM character_inventory WHERE guid = 88888888")
        cur.close()
        c.close()

@run_test("FK_OrphanedSpellPrevented")
def test_fk_spell():
    """Cannot insert spells for a non-existent character."""
    c = conn("character3")
    cur = c.cursor()
    try:
        cur.execute(
            "INSERT INTO character_spell (guid, spell, active, disabled) "
            "VALUES (88888888, 133, 1, 0)"  # guid 88888888 doesn't exist
        )
        cur.execute("DELETE FROM character_spell WHERE guid = 88888888")
        assert False, "FK constraint on character_spell not enforced"
    except (db_driver.errors.DatabaseError, db_driver.errors.IntegrityError):
        pass
    finally:
        cur.execute("DELETE FROM character_spell WHERE guid = 88888888")
        cur.close()
        c.close()

@run_test("FK_CascadeDeleteWorks")
def test_fk_cascade():
    """Deleting a character cascades to related tables."""
    c = conn("character3", autocommit=False)
    cur = c.cursor()
    try:
        # Create a character with spells and quest status
        cur.execute(
            "INSERT INTO characters (guid, account, name, race, class, level) "
            "VALUES (777777, 1, 'CascadeTest', 1, 1, 1)"
        )
        cur.execute(
            "INSERT INTO character_spell (guid, spell, active, disabled) "
            "VALUES (777777, 133, 1, 0)"
        )
        cur.execute(
            "INSERT INTO character_queststatus (guid, quest, status) "
            "VALUES (777777, 1, 1)"
        )
        c.commit()

        # Delete the character
        cur.execute("DELETE FROM characters WHERE guid = 777777")
        c.commit()

        # Verify cascade deleted related records
        cur.execute("SELECT COUNT(*) FROM character_spell WHERE guid = 777777")
        assert cur.fetchone()[0] == 0, "character_spell cascade delete failed"
        cur.execute("SELECT COUNT(*) FROM character_queststatus WHERE guid = 777777")
        assert cur.fetchone()[0] == 0, "character_queststatus cascade delete failed"
    except Exception:
        c.rollback()
        raise
    finally:
        cur.execute("DELETE FROM characters WHERE guid = 777777")
        c.commit()
        cur.close()
        c.close()


# ═══════════════════════════════════════════════════════════════════════════
# 4. SECURITY TABLES EXIST
# ═══════════════════════════════════════════════════════════════════════════

@run_test("Security_LoginAttemptTableExists")
def test_login_attempts_table():
    """account_login_attempts table must exist for brute-force tracking."""
    c = conn("realmd")
    cur = c.cursor()
    cur.execute("SHOW TABLES LIKE 'account_login_attempts'")
    assert cur.fetchone() is not None, "account_login_attempts table missing"
    # Verify it can store records
    cur.execute(
        "INSERT INTO account_login_attempts (account_id, ip, success) "
        "VALUES (1, '192.168.1.100', 0)"
    )
    cur.execute("SELECT COUNT(*) FROM account_login_attempts WHERE ip = '192.168.1.100'")
    assert cur.fetchone()[0] >= 1
    cur.execute("DELETE FROM account_login_attempts WHERE ip = '192.168.1.100'")
    cur.close()
    c.close()

@run_test("Security_RateLimitTableExists")
def test_rate_limit_table():
    """ip_rate_limit table must exist for throttling."""
    c = conn("realmd")
    cur = c.cursor()
    cur.execute("SHOW TABLES LIKE 'ip_rate_limit'")
    assert cur.fetchone() is not None, "ip_rate_limit table missing"
    cur.close()
    c.close()

@run_test("Security_AuditLogTableExists")
def test_audit_log_table():
    """security_audit_log table must exist in character3."""
    c = conn("character3")
    cur = c.cursor()
    cur.execute("SHOW TABLES LIKE 'security_audit_log'")
    assert cur.fetchone() is not None, "security_audit_log table missing"
    # Can write audit entries
    cur.execute(
        "INSERT INTO security_audit_log (account_id, char_guid, action, details, ip) "
        "VALUES (1, 0, 'TEST', 'security hardening validation', '127.0.0.1')"
    )
    cur.execute("DELETE FROM security_audit_log WHERE action = 'TEST'")
    cur.close()
    c.close()


# ═══════════════════════════════════════════════════════════════════════════
# 5. SQL INJECTION — verify parameterized queries block injection
# ═══════════════════════════════════════════════════════════════════════════

@run_test("SQLi_ClassicDropTable_Blocked")
def test_sqli_drop():
    """'; DROP TABLE account; -- must not execute."""
    c = conn("realmd")
    cur = c.cursor()
    evil = "'; DROP TABLE account; --"
    cur.execute("SELECT * FROM account WHERE username = %s", (evil,))
    cur.fetchall()
    # Table must still exist
    cur.execute("SELECT COUNT(*) FROM account")
    count = cur.fetchone()[0]
    assert count >= 2, f"account table damaged after injection! Only {count} rows"
    cur.close()
    c.close()

@run_test("SQLi_UnionSelect_Blocked")
def test_sqli_union():
    """UNION SELECT attack on account credentials must not leak data."""
    c = conn("realmd")
    cur = c.cursor()
    evil = "' UNION SELECT id, sha_pass_hash, sha_pass_hash, sha_pass_hash, sha_pass_hash FROM account-- "
    cur.execute("SELECT username FROM account WHERE username = %s", (evil,))
    rows = cur.fetchall()
    # Should return 0 rows (no account with that exact username)
    assert len(rows) == 0, f"UNION SELECT returned {len(rows)} rows — possible injection"
    cur.close()
    c.close()

@run_test("SQLi_BooleanBlind_Blocked")
def test_sqli_boolean():
    """Boolean blind injection must not bypass auth."""
    c = conn("realmd")
    cur = c.cursor()
    # If parameterized, this literally looks for username "' OR '1'='1"
    evil = "' OR '1'='1"
    cur.execute("SELECT COUNT(*) FROM account WHERE username = %s AND sha_pass_hash = %s",
                (evil, "wronghash"))
    count = cur.fetchone()[0]
    assert count == 0, f"Boolean injection returned {count} rows — auth bypass possible"
    cur.close()
    c.close()

@run_test("SQLi_TimeBasedBlind_Blocked")
def test_sqli_timebased():
    """Time-based injection (SLEEP/BENCHMARK) must not cause delays."""
    c = conn("realmd")
    cur = c.cursor()
    evil = "'; SELECT SLEEP(5); --"
    t0 = time.time()
    cur.execute("SELECT * FROM account WHERE username = %s", (evil,))
    cur.fetchall()
    elapsed = time.time() - t0
    assert elapsed < 3.0, f"Time-based injection caused {elapsed:.1f}s delay — injection possible"
    cur.close()
    c.close()

@run_test("SQLi_SecondOrder_CharacterName")
def test_sqli_second_order():
    """Second-order injection: store malicious name, retrieve safely."""
    c = conn("character3")
    cur = c.cursor()
    evil_name = "'; UPDATE characters SET level=255 WHERE 1=1; --"
    # Clean up any leftover from prior runs
    cur.execute("DELETE FROM characters WHERE guid = 999994")
    cur.fetchall()
    try:
        cur.execute(
            "INSERT INTO characters (guid, account, name, race, class, level) "
            "VALUES (999994, 1, %s, 1, 1, 1)",
            (evil_name[:12],)  # name col is VARCHAR(12) — truncated
        )
        cur.fetchall()  # consume any result
        cur.execute("SELECT name FROM characters WHERE guid = 999994")
        row = cur.fetchone()
        cur.fetchall()  # drain
        # Retrieve and use in another query safely
        if row:
            stored_name = row[0]
            cur.execute("SELECT level FROM characters WHERE name = %s", (stored_name,))
            cur.fetchall()  # drain
        # All characters must still be level <= 85 (constraint enforced)
        cur.execute("SELECT MAX(level) FROM characters")
        max_level = cur.fetchone()[0]
        assert max_level is None or max_level <= 85, \
            f"Level was modified by second-order injection: {max_level}"
    finally:
        cur.execute("DELETE FROM characters WHERE guid = 999994")
        cur.fetchall()
        cur.close()
        c.close()


# ═══════════════════════════════════════════════════════════════════════════
# 6. GOLD / ECONOMY EXPLOIT RESISTANCE
# ═══════════════════════════════════════════════════════════════════════════

@run_test("Economy_MoneyIntegerOverflow_Blocked")
def test_money_overflow():
    """Money values beyond MAX_MONEY_AMOUNT must be blocked by CHECK constraint."""
    c = conn("character3")
    cur = c.cursor()
    cur.execute(
        "INSERT INTO characters (guid, account, name, race, class, level, money) "
        "VALUES (999995, 1, 'OverflTest', 1, 1, 1, 9999999999)"
    )
    cur.fetchall()
    # Try to UPDATE to a value exceeding the CHECK constraint cap
    try:
        cur.execute("UPDATE characters SET money = 99999999999 WHERE guid = 999995")
        cur.fetchall()
        cur.execute("SELECT money FROM characters WHERE guid = 999995")
        val = cur.fetchone()[0]
        assert val <= 9999999999, f"Money cap CHECK constraint not enforced: {val}"
    except (db_driver.errors.DatabaseError, db_driver.errors.IntegrityError):
        pass  # constraint blocked it — good
    finally:
        cur.execute("DELETE FROM characters WHERE guid = 999995")
        cur.fetchall()
        cur.close()
        c.close()

@run_test("Economy_NegativeMoney_Blocked")
def test_negative_money():
    """Negative money value must be blocked (UNSIGNED column)."""
    c = conn("character3")
    cur = c.cursor()
    try:
        cur.execute(
            "INSERT INTO characters (guid, account, name, race, class, level, money) "
            "VALUES (999996, 1, 'NegMoney', 1, 1, 1, -1)"
        )
        cur.execute("SELECT money FROM characters WHERE guid = 999996")
        val = cur.fetchone()[0]
        # UNSIGNED wraps around — check it wasn't stored as negative
        assert val >= 0, f"Negative money stored as: {val}"
    except (db_driver.errors.DatabaseError, db_driver.errors.IntegrityError,
            db_driver.errors.DataError):
        pass  # expected — UNSIGNED prevents negatives
    finally:
        cur.execute("DELETE FROM characters WHERE guid = 999996")
        cur.close()
        c.close()

@run_test("Economy_ConcurrentMoneyUpdate_Consistent")
def test_concurrent_money():
    """Concurrent money updates must not cause lost-update races."""
    c = conn("character3")
    cur = c.cursor()
    cur.execute(
        "INSERT INTO characters (guid, account, name, race, class, level, money) "
        "VALUES (999997, 1, 'MoneyRace', 1, 1, 1, 0)"
    )
    c.commit()

    # 20 threads each add 100 gold using SELECT ... FOR UPDATE
    errors = []
    def add_gold(amount):
        try:
            tc = conn("character3", autocommit=False)
            tcur = tc.cursor()
            tcur.execute("SELECT money FROM characters WHERE guid = 999997 FOR UPDATE")
            current = tcur.fetchone()[0]
            new_val = min(current + amount, 9999999999)
            tcur.execute("UPDATE characters SET money = %s WHERE guid = 999997", (new_val,))
            tc.commit()
            tcur.close()
            tc.close()
        except Exception as e:
            errors.append(str(e))

    threads = [threading.Thread(target=add_gold, args=(100,)) for _ in range(20)]
    for t in threads: t.start()
    for t in threads: t.join(timeout=15)

    cur.execute("SELECT money FROM characters WHERE guid = 999997")
    final = cur.fetchone()[0]
    cur.execute("DELETE FROM characters WHERE guid = 999997")
    c.commit()
    cur.close()
    c.close()
    assert final == 2000, f"Concurrent gold add: expected 2000, got {final} ({len(errors)} errors)"


# ═══════════════════════════════════════════════════════════════════════════
# 7. NORMAL GAME ACTIONS — regression checks post-hardening
# ═══════════════════════════════════════════════════════════════════════════

@run_test("NormalAction_CreateCharacter")
def test_normal_create_char():
    """Normal character creation workflow must still work."""
    c = conn("character3")
    cur = c.cursor()
    cur.execute(
        "INSERT INTO characters (guid, account, name, race, class, gender, level, "
        "xp, money, map, position_x, position_y, position_z) "
        "VALUES (888001, 1, 'Testchar', 1, 1, 0, 1, 0, 0, 0, -8949.95, -132.493, 83.5)"
    )
    cur.execute("SELECT guid, name, level FROM characters WHERE guid = 888001")
    row = cur.fetchone()
    assert row is not None and row[1] == "Testchar" and row[2] == 1
    cur.execute("DELETE FROM characters WHERE guid = 888001")
    cur.close()
    c.close()

@run_test("NormalAction_AddSpellToCharacter")
def test_normal_add_spell():
    """Adding a spell to a character must work with FK constraints."""
    c = conn("character3", autocommit=False)
    cur = c.cursor()
    # Create character first
    cur.execute(
        "INSERT INTO characters (guid, account, name, race, class, level) "
        "VALUES (888002, 1, 'SpellTest', 1, 1, 1)"
    )
    cur.execute(
        "INSERT INTO character_spell (guid, spell, active, disabled) "
        "VALUES (888002, 133, 1, 0)"  # Fireball
    )
    c.commit()
    cur.execute("SELECT spell FROM character_spell WHERE guid = 888002")
    assert cur.fetchone()[0] == 133
    cur.execute("DELETE FROM characters WHERE guid = 888002")
    c.commit()
    # spell should cascade-delete
    cur.execute("SELECT COUNT(*) FROM character_spell WHERE guid = 888002")
    assert cur.fetchone()[0] == 0, "Spell not cascade-deleted with character"
    cur.close()
    c.close()

@run_test("NormalAction_MailSystem")
def test_normal_mail():
    """Mail with valid money amount must be insertable."""
    c = conn("character3")
    cur = c.cursor()
    cur.execute(
        "INSERT INTO mail (id, messageType, stationery, sender, receiver, subject, "
        "body, money, expire_time, deliver_time) "
        "VALUES (888003, 0, 41, 1, 2, 'Hello', 'Test mail', 100, "
        "UNIX_TIMESTAMP() + 86400, UNIX_TIMESTAMP())"
    )
    cur.execute("SELECT money FROM mail WHERE id = 888003")
    assert cur.fetchone()[0] == 100
    cur.execute("DELETE FROM mail WHERE id = 888003")
    cur.close()
    c.close()

@run_test("NormalAction_AccountLookup")
def test_normal_account_lookup():
    """Account lookup by username must work correctly."""
    c = conn("realmd")
    cur = c.cursor()
    cur.execute("SELECT id, username, gmlevel FROM account WHERE username = 'TEST'")
    row = cur.fetchone()
    assert row is not None, "TEST account not found"
    assert row[1] == "TEST"
    assert row[2] == 3, f"TEST should have gmlevel 3, got {row[2]}"
    cur.close()
    c.close()

@run_test("NormalAction_RealmlistQuery")
def test_normal_realmlist():
    """Realm list query must return expected realm."""
    c = conn("realmd")
    cur = c.cursor()
    cur.execute("SELECT name, address, port FROM realmlist WHERE id = 1")
    row = cur.fetchone()
    assert row is not None, "No realm in realmlist"
    assert row[0] == "MaNGOS Three"
    assert row[1] == "127.0.0.1"
    assert row[2] == 8085
    cur.close()
    c.close()

@run_test("NormalAction_GuildCreate")
def test_normal_guild():
    """Guild creation must work with name constraints."""
    c = conn("character3")
    cur = c.cursor()
    cur.execute(
        "INSERT INTO guild (guildid, name, leaderguid, info, motd, createdate) "
        "VALUES (888004, 'TestGuild', 1, 'A test guild', 'Welcome!', UNIX_TIMESTAMP())"
    )
    cur.execute("SELECT name FROM guild WHERE guildid = 888004")
    assert cur.fetchone()[0] == "TestGuild"
    cur.execute("DELETE FROM guild WHERE guildid = 888004")
    cur.close()
    c.close()

@run_test("NormalAction_GuildNameTooLong_Blocked")
def test_guild_name_too_long():
    """Guild names longer than 24 chars must be rejected."""
    c = conn("character3")
    cur = c.cursor()
    try:
        cur.execute(
            "INSERT INTO guild (guildid, name, leaderguid, info, motd, createdate) "
            "VALUES (888005, 'ThisGuildNameIsTooLongToBeValid', 1, '', '', 0)"
        )
        cur.execute("DELETE FROM guild WHERE guildid = 888005")
        assert False, "Guild name length constraint not enforced"
    except (db_driver.errors.DatabaseError, db_driver.errors.IntegrityError):
        pass
    finally:
        cur.execute("DELETE FROM guild WHERE guildid = 888005")
        cur.close()
        c.close()


# ═══════════════════════════════════════════════════════════════════════════
# 8. ACCOUNT SECURITY
# ═══════════════════════════════════════════════════════════════════════════

@run_test("AccountSec_BanTableWorks")
def test_ban_table():
    """Account ban table must properly record bans."""
    c = conn("realmd")
    cur = c.cursor()
    now = int(time.time())
    cur.execute(
        "INSERT INTO account_banned (id, bandate, unbandate, bannedby, banreason, active) "
        "VALUES (1, %s, %s, 'admin', 'security test', 1)",
        (now, now + 3600)
    )
    cur.execute("SELECT active FROM account_banned WHERE id = 1 AND bandate = %s", (now,))
    assert cur.fetchone()[0] == 1, "Ban not recorded"
    cur.execute("DELETE FROM account_banned WHERE id = 1 AND bandate = %s", (now,))
    cur.close()
    c.close()

@run_test("AccountSec_IPBanTableWorks")
def test_ip_ban_table():
    """IP ban table must properly record bans."""
    c = conn("realmd")
    cur = c.cursor()
    now = int(time.time())
    cur.execute(
        "INSERT INTO ip_banned (ip, bandate, unbandate, bannedby, banreason) "
        "VALUES ('10.0.0.1', %s, %s, 'admin', 'exploit attempt')",
        (now, now + 86400)
    )
    cur.execute("SELECT banreason FROM ip_banned WHERE ip = '10.0.0.1' AND bandate = %s", (now,))
    assert cur.fetchone()[0] == "exploit attempt"
    cur.execute("DELETE FROM ip_banned WHERE ip = '10.0.0.1' AND bandate = %s", (now,))
    cur.close()
    c.close()

@run_test("AccountSec_PasswordHashNeverPlaintext")
def test_password_not_plaintext():
    """Stored password hash must not be the plaintext password."""
    c = conn("realmd")
    cur = c.cursor()
    cur.execute("SELECT sha_pass_hash FROM account WHERE username = 'TEST'")
    row = cur.fetchone()
    assert row is not None
    stored_hash = row[0]
    assert stored_hash != "TEST", "Password stored as plaintext!"
    assert stored_hash != "test", "Password stored as lowercase plaintext!"
    assert len(stored_hash) == 40, f"SHA1 hash should be 40 chars, got {len(stored_hash)}"
    cur.close()
    c.close()

@run_test("AccountSec_LoginAttemptTracking")
def test_login_attempt_tracking():
    """Login attempts should be trackable for brute-force detection."""
    c = conn("realmd")
    cur = c.cursor()
    # Simulate 5 failed login attempts
    for i in range(5):
        cur.execute(
            "INSERT INTO account_login_attempts (account_id, ip, success) "
            "VALUES (1, '10.0.0.99', 0)"
        )
    cur.execute(
        "SELECT COUNT(*) FROM account_login_attempts "
        "WHERE account_id = 1 AND ip = '10.0.0.99' AND success = 0"
    )
    count = cur.fetchone()[0]
    assert count == 5, f"Expected 5 failed attempts, found {count}"
    cur.execute("DELETE FROM account_login_attempts WHERE ip = '10.0.0.99'")
    cur.close()
    c.close()


# ═══════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════

def main():
    print("=" * 66)
    print("  MaNGOS Three — Security Hardening Validation Suite")
    print(f"  Target: {DB_USER}@{DB_HOST}:{DB_PORT}")
    print("=" * 66)

    # Verify connectivity
    try:
        c = conn("realmd")
        cur = c.cursor()
        cur.execute("SELECT VERSION()")
        ver = cur.fetchone()[0]
        print(f"  MariaDB: {ver}")
        cur.close()
        c.close()
    except Exception as e:
        print(f"ERROR: Cannot connect: {e}")
        sys.exit(1)

    print()

    # Collect and run tests in order
    tests = [obj for name, obj in globals().items()
             if callable(obj) and getattr(obj, "_test", False)]

    sections = [
        ("Privilege Reduction", [t for t in tests if t.__name__.startswith("Privilege_") or "priv" in t.__name__]),
        ("CHECK Constraints",   [t for t in tests if t.__name__.startswith("Constraint_")]),
        ("Foreign Key Integrity",[t for t in tests if t.__name__.startswith("FK_")]),
        ("Security Tables",     [t for t in tests if t.__name__.startswith("Security_")]),
        ("SQL Injection",       [t for t in tests if t.__name__.startswith("SQLi_")]),
        ("Economy Exploits",    [t for t in tests if t.__name__.startswith("Economy_")]),
        ("Normal Game Actions", [t for t in tests if t.__name__.startswith("NormalAction_")]),
        ("Account Security",    [t for t in tests if t.__name__.startswith("AccountSec_")]),
    ]

    for section_name, section_tests in sections:
        if not section_tests:
            continue
        print(f"── {section_name} {'─' * (50 - len(section_name))}")
        for t in section_tests:
            t()
        print()

    passed = RESULTS["passed"]
    failed = RESULTS["failed"]
    total  = passed + failed

    print("=" * 66)
    print(f"  Results: {passed} passed, {failed} failed, {total} total")
    if RESULTS["errors"]:
        print("\n  Failures:")
        for e in RESULTS["errors"]:
            print(f"    - {e}")
    print("=" * 66)

    # Final DB health check
    try:
        c = conn("realmd")
        cur = c.cursor()
        cur.execute("SELECT COUNT(*) FROM account")
        count = cur.fetchone()[0]
        cur.close()
        c.close()
        print(f"\n  MariaDB status: ALIVE ({count} accounts)")
    except Exception as e:
        print(f"\n  MariaDB status: ERROR — {e}")

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
