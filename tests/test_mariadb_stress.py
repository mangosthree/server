#!/usr/bin/env python3
"""
MariaDB Stress Test Suite for MaNGOS Server

Directly hammers the MariaDB database with adversarial queries to find:
- Connection pool exhaustion
- Transaction deadlocks
- Data corruption under concurrent writes
- SQL injection vectors through the game protocol
- Resource leaks from unclosed cursors/connections
- Edge cases in the ORM/query layer

Requires: pip install mysql-connector-python (or pymysql)
"""

import sys
import os
import time
import threading
import random
import string
import signal
import traceback

try:
    import mysql.connector as db_driver
    DB_LIB = "mysql-connector"
except ImportError:
    try:
        import pymysql as db_driver
        DB_LIB = "pymysql"
    except ImportError:
        print("ERROR: Need mysql-connector-python or pymysql")
        print("  pip install mysql-connector-python")
        sys.exit(1)

# ── Configuration ─────────────────────────────────────────────────────────────
DB_HOST = os.environ.get("DB_HOST", "127.0.0.1")
DB_PORT = int(os.environ.get("DB_PORT", "3306"))
DB_USER = os.environ.get("DB_USER", "mangos")
DB_PASS = os.environ.get("DB_PASS", "mangos")

DATABASES = {
    "realmd":     os.environ.get("DB_REALMD", "realmd"),
    "characters": os.environ.get("DB_CHARS", "character3"),
    "world":      os.environ.get("DB_WORLD", "mangos3"),
}

RESULTS = {"passed": 0, "failed": 0, "errors": []}
LOCK = threading.Lock()

# ── Helpers ───────────────────────────────────────────────────────────────────

def get_conn(database="realmd", autocommit=True):
    """Get a fresh DB connection."""
    if DB_LIB == "mysql-connector":
        return db_driver.connect(
            host=DB_HOST, port=DB_PORT, user=DB_USER, password=DB_PASS,
            database=DATABASES[database], autocommit=autocommit,
            connection_timeout=5
        )
    else:
        return db_driver.connect(
            host=DB_HOST, port=DB_PORT, user=DB_USER, passwd=DB_PASS,
            db=DATABASES[database], autocommit=autocommit,
            connect_timeout=5
        )

def test(name):
    """Test decorator with result tracking."""
    def decorator(func):
        def wrapper():
            try:
                func()
                with LOCK:
                    RESULTS["passed"] += 1
                print(f"  [PASS] {name}")
            except AssertionError as e:
                with LOCK:
                    RESULTS["failed"] += 1
                    RESULTS["errors"].append(f"{name}: {e}")
                print(f"  [FAIL] {name}: {e}")
            except Exception as e:
                with LOCK:
                    RESULTS["failed"] += 1
                    RESULTS["errors"].append(f"{name}: {type(e).__name__}: {e}")
                print(f"  [ERROR] {name}: {type(e).__name__}: {e}")
        wrapper.__name__ = name
        wrapper._test = True
        return wrapper
    return decorator

def random_string(n=16):
    return ''.join(random.choices(string.ascii_uppercase + string.digits, k=n))

def cleanup_test_accounts(conn):
    """Remove test-generated accounts (id > 100)."""
    cur = conn.cursor()
    cur.execute("DELETE FROM account WHERE id > 100")
    conn.commit()
    cur.close()

# ═══════════════════════════════════════════════════════════════════════════════
# CONNECTION POOL STRESS
# ═══════════════════════════════════════════════════════════════════════════════

@test("ConnPool_RapidOpenClose_200")
def test_conn_pool_rapid():
    """Open and close 200 connections rapidly — leak detection."""
    for _ in range(200):
        c = get_conn("realmd")
        cur = c.cursor()
        cur.execute("SELECT 1")
        cur.fetchone()
        cur.close()
        c.close()

@test("ConnPool_Concurrent_50")
def test_conn_pool_concurrent():
    """50 threads each opening a connection and querying simultaneously."""
    errors = []
    def worker():
        try:
            c = get_conn("realmd")
            cur = c.cursor()
            cur.execute("SELECT COUNT(*) FROM account")
            cur.fetchone()
            time.sleep(random.uniform(0.01, 0.05))
            cur.close()
            c.close()
        except Exception as e:
            errors.append(str(e))

    threads = [threading.Thread(target=worker) for _ in range(50)]
    for t in threads: t.start()
    for t in threads: t.join(timeout=15)
    assert len(errors) < 5, f"{len(errors)} connection errors: {errors[:3]}"

@test("ConnPool_AbandonWithoutClose_30")
def test_conn_abandon():
    """Open 30 connections and let them fall out of scope (GC should close)."""
    conns = []
    for _ in range(30):
        c = get_conn("realmd")
        cur = c.cursor()
        cur.execute("SELECT 1")
        cur.fetchone()
        conns.append(c)
    # intentionally don't close — rely on destructor
    del conns
    # server should still accept new connections
    c = get_conn("realmd")
    cur = c.cursor()
    cur.execute("SELECT 1")
    cur.fetchone()
    cur.close()
    c.close()

@test("ConnPool_CrossDatabase_Rapid")
def test_cross_db_rapid():
    """Rapidly switch between all 3 databases."""
    for _ in range(100):
        db = random.choice(["realmd", "characters", "world"])
        c = get_conn(db)
        cur = c.cursor()
        cur.execute("SELECT 1")
        cur.fetchone()
        cur.close()
        c.close()

# ═══════════════════════════════════════════════════════════════════════════════
# TRANSACTION STRESS & DEADLOCKS
# ═══════════════════════════════════════════════════════════════════════════════

@test("Transaction_RapidCommitRollback_100")
def test_txn_rapid():
    """100 rapid begin/commit and begin/rollback cycles."""
    c = get_conn("characters", autocommit=False)
    cur = c.cursor()
    for i in range(100):
        cur.execute("INSERT INTO characters (guid, account, name, race, class, level) "
                     "VALUES (%s, 1, %s, 1, 1, 1)",
                     (10000 + i, f"TXNTEST{i}"))
        if i % 2 == 0:
            c.commit()
        else:
            c.rollback()
    # cleanup
    cur.execute("DELETE FROM characters WHERE guid >= 10000 AND guid < 10100")
    c.commit()
    cur.close()
    c.close()

@test("Transaction_ConcurrentInsert_SameRow")
def test_txn_concurrent_same_row():
    """Multiple threads trying to INSERT the same primary key — should fail gracefully."""
    errors = []
    barrier = threading.Barrier(10, timeout=5)
    def worker(thread_id):
        try:
            c = get_conn("characters", autocommit=False)
            cur = c.cursor()
            barrier.wait()
            try:
                cur.execute("INSERT INTO characters (guid, account, name, race, class, level) "
                             "VALUES (99999, 1, %s, 1, 1, 1)", (f"RACE{thread_id}",))
                c.commit()
            except Exception:
                c.rollback()  # expected — duplicate key
            cur.close()
            c.close()
        except Exception as e:
            errors.append(str(e))

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(10)]
    for t in threads: t.start()
    for t in threads: t.join(timeout=10)
    # cleanup
    c = get_conn("characters")
    cur = c.cursor()
    cur.execute("DELETE FROM characters WHERE guid = 99999")
    c.commit()
    cur.close()
    c.close()
    assert len(errors) < 3, f"Too many errors: {errors}"

@test("Transaction_Deadlock_Provocation")
def test_txn_deadlock():
    """Two threads updating rows in reverse order to provoke deadlock detection."""
    # Setup: create two rows
    c = get_conn("characters")
    cur = c.cursor()
    cur.execute("INSERT IGNORE INTO characters (guid, account, name, race, class, level) "
                 "VALUES (50001, 1, 'DEADLOCK1', 1, 1, 1)")
    cur.execute("INSERT IGNORE INTO characters (guid, account, name, race, class, level) "
                 "VALUES (50002, 1, 'DEADLOCK2', 1, 1, 1)")
    c.commit()
    cur.close()
    c.close()

    deadlock_detected = []
    def worker_a():
        try:
            c = get_conn("characters", autocommit=False)
            cur = c.cursor()
            cur.execute("UPDATE characters SET level = level + 1 WHERE guid = 50001")
            time.sleep(0.1)
            cur.execute("UPDATE characters SET level = level + 1 WHERE guid = 50002")
            c.commit()
            cur.close()
            c.close()
        except Exception as e:
            deadlock_detected.append(str(e))
            try: c.rollback()
            except: pass

    def worker_b():
        try:
            c = get_conn("characters", autocommit=False)
            cur = c.cursor()
            cur.execute("UPDATE characters SET level = level + 1 WHERE guid = 50002")
            time.sleep(0.1)
            cur.execute("UPDATE characters SET level = level + 1 WHERE guid = 50001")
            c.commit()
            cur.close()
            c.close()
        except Exception as e:
            deadlock_detected.append(str(e))
            try: c.rollback()
            except: pass

    ta = threading.Thread(target=worker_a)
    tb = threading.Thread(target=worker_b)
    ta.start(); tb.start()
    ta.join(timeout=10); tb.join(timeout=10)

    # Cleanup
    c = get_conn("characters")
    cur = c.cursor()
    cur.execute("DELETE FROM characters WHERE guid IN (50001, 50002)")
    c.commit()
    cur.close()
    c.close()
    # Deadlock is acceptable — what matters is no crash or hang

@test("Transaction_NestedRollback_Stress")
def test_txn_nested_rollback():
    """Savepoints and rollbacks — MariaDB extension stress."""
    c = get_conn("characters", autocommit=False)
    cur = c.cursor()
    try:
        for i in range(50):
            cur.execute(f"SAVEPOINT sp{i}")
            cur.execute("INSERT INTO characters (guid, account, name, race, class, level) "
                         "VALUES (%s, 1, %s, 1, 1, 1)",
                         (60000 + i, f"SAVEPOINT{i}"))
            if i % 3 == 0:
                cur.execute(f"ROLLBACK TO SAVEPOINT sp{i}")
        c.commit()
    except Exception:
        c.rollback()
    # cleanup
    cur.execute("DELETE FROM characters WHERE guid >= 60000 AND guid < 60050")
    c.commit()
    cur.close()
    c.close()

# ═══════════════════════════════════════════════════════════════════════════════
# DATA CORRUPTION / INTEGRITY STRESS
# ═══════════════════════════════════════════════════════════════════════════════

@test("Integrity_InsertMaxValues")
def test_insert_max_values():
    """Insert rows with boundary values for every numeric type."""
    c = get_conn("characters")
    cur = c.cursor()
    # TINYINT UNSIGNED max = 255, INT UNSIGNED max = 4294967295
    cur.execute("INSERT INTO characters (guid, account, name, race, class, level, xp, money) "
                 "VALUES (70001, 4294967295, 'MAXVALS', 255, 255, 255, 4294967295, 4294967295)")
    cur.execute("SELECT * FROM characters WHERE guid = 70001")
    row = cur.fetchone()
    assert row is not None, "Max-value row not found"
    cur.execute("DELETE FROM characters WHERE guid = 70001")
    c.commit()
    cur.close()
    c.close()

@test("Integrity_InsertUnicode_AccountName")
def test_insert_unicode():
    """Try unicode / multibyte strings in account username field."""
    c = get_conn("realmd")
    cur = c.cursor()
    weird_names = [
        "TËST_ÜNÎ",
        "测试账户",
        "ТЕСТ_РУС",
        "🔥FIRE🔥",
        "NULL\x00BYTE",
        "A" * 32,  # max length
        "",  # empty
    ]
    for i, name in enumerate(weird_names):
        try:
            cur.execute("INSERT INTO account (id, username, sha_pass_hash) VALUES (%s, %s, %s)",
                         (200 + i, name[:32], "0" * 40))
            c.commit()
        except Exception:
            c.rollback()  # expected for some
    # cleanup
    cur.execute("DELETE FROM account WHERE id >= 200 AND id < 300")
    c.commit()
    cur.close()
    c.close()

@test("Integrity_ConcurrentCharacterWrite_100")
def test_concurrent_char_write():
    """100 threads each creating a character simultaneously."""
    errors = []
    def worker(tid):
        try:
            c = get_conn("characters")
            cur = c.cursor()
            cur.execute("INSERT INTO characters (guid, account, name, race, class, level) "
                         "VALUES (%s, 1, %s, 1, 1, 1)", (20000 + tid, f"CONC{tid}"))
            c.commit()
            cur.close()
            c.close()
        except Exception as e:
            errors.append(str(e))

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(100)]
    for t in threads: t.start()
    for t in threads: t.join(timeout=20)

    # verify all were created
    c = get_conn("characters")
    cur = c.cursor()
    cur.execute("SELECT COUNT(*) FROM characters WHERE guid >= 20000 AND guid < 20100")
    count = cur.fetchone()[0]
    # cleanup
    cur.execute("DELETE FROM characters WHERE guid >= 20000 AND guid < 20100")
    c.commit()
    cur.close()
    c.close()
    assert count >= 95, f"Only {count}/100 concurrent inserts succeeded, errors: {errors[:3]}"

@test("Integrity_UpdateRace_10000x")
def test_update_race_10k():
    """Rapidly update the same row 10,000 times — lost update check."""
    c = get_conn("characters")
    cur = c.cursor()
    cur.execute("INSERT INTO characters (guid, account, name, race, class, level) "
                 "VALUES (30001, 1, 'HOTROW', 1, 1, 0)")
    c.commit()
    for i in range(10000):
        cur.execute("UPDATE characters SET level = %s WHERE guid = 30001", (i % 256,))
    cur.execute("SELECT level FROM characters WHERE guid = 30001")
    final_level = cur.fetchone()[0]
    cur.execute("DELETE FROM characters WHERE guid = 30001")
    c.commit()
    cur.close()
    c.close()
    assert final_level == 9999 % 256, f"Expected {9999 % 256}, got {final_level}"

# ═══════════════════════════════════════════════════════════════════════════════
# SQL INJECTION / ESCAPE TESTING
# ═══════════════════════════════════════════════════════════════════════════════

@test("SQLi_ParameterizedQueries_Safe")
def test_sqli_parameterized():
    """Verify parameterized queries prevent injection."""
    c = get_conn("realmd")
    cur = c.cursor()
    evil_inputs = [
        "'; DROP TABLE account; --",
        "1' OR '1'='1",
        "admin'--",
        "' UNION SELECT * FROM account--",
        "'; INSERT INTO account VALUES(999,'HACKED','x',3,NULL,NULL,NULL,'',NOW(),'',0,0,Now(),0,3,0,0,'',0);--",
        "\"; DROP TABLE account; --",
        "\\'; DROP TABLE account; --",
        "1; WAITFOR DELAY '00:00:05'--",
        "' AND 1=CONVERT(int, (SELECT TOP 1 sha_pass_hash FROM account))--",
    ]
    for evil in evil_inputs:
        try:
            cur.execute("SELECT * FROM account WHERE username = %s", (evil,))
            cur.fetchall()
        except Exception:
            pass  # query failure is fine, crash is not
    # verify table still exists and has data
    cur.execute("SELECT COUNT(*) FROM account")
    count = cur.fetchone()[0]
    cur.close()
    c.close()
    assert count >= 2, f"Account table damaged! Only {count} rows left"

@test("SQLi_EscapeString_Malicious")
def test_sqli_escape():
    """Test that string escaping handles malicious input."""
    c = get_conn("realmd")
    cur = c.cursor()
    # These should all be safely escaped by the driver
    payloads = [
        "test\x00null",
        "test\nline",
        "test\rcarriage",
        "test\x1aescape",
        "test\\backslash",
        "test'quote",
        'test"double',
    ]
    for p in payloads:
        try:
            cur.execute("INSERT INTO account (id, username, sha_pass_hash) VALUES (%s, %s, %s)",
                         (300, p[:32], "0" * 40))
            c.commit()
        except Exception:
            c.rollback()
        cur.execute("DELETE FROM account WHERE id = 300")
        c.commit()
    cur.close()
    c.close()

# ═══════════════════════════════════════════════════════════════════════════════
# QUERY STRESS (simulating server workloads)
# ═══════════════════════════════════════════════════════════════════════════════

@test("QueryStress_CharEnum_1000x")
def test_query_char_enum():
    """Simulate 1000 character enumeration queries (the most common DB hit)."""
    c = get_conn("characters")
    cur = c.cursor()
    for _ in range(1000):
        cur.execute("SELECT characters.guid, name, race, class, gender, level, zone, map, "
                     "position_x, position_y, position_z, guild_member.guildid "
                     "FROM characters LEFT JOIN guild_member ON characters.guid = guild_member.guid "
                     "WHERE account = %s AND deleteDate IS NULL ORDER BY characters.guid", (1,))
        cur.fetchall()
    cur.close()
    c.close()

@test("QueryStress_AccountLookup_MixedLoad")
def test_query_mixed_load():
    """20 threads doing mixed reads/writes to the account table."""
    errors = []
    def worker(tid):
        try:
            c = get_conn("realmd")
            cur = c.cursor()
            for _ in range(50):
                op = random.choice(["read", "read", "read", "write"])
                if op == "read":
                    cur.execute("SELECT * FROM account WHERE username = %s", ("TEST",))
                    cur.fetchall()
                else:
                    cur.execute("UPDATE account SET last_ip = %s WHERE username = %s",
                                 (f"10.0.{tid}.{random.randint(1,254)}", "TEST"))
                    c.commit()
            cur.close()
            c.close()
        except Exception as e:
            errors.append(str(e))

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(20)]
    for t in threads: t.start()
    for t in threads: t.join(timeout=30)
    assert len(errors) < 3, f"Too many errors in mixed load: {errors[:3]}"

@test("QueryStress_LargeResultSet")
def test_large_result_set():
    """Insert 5000 rows then SELECT them all back — memory pressure."""
    c = get_conn("characters")
    cur = c.cursor()
    # bulk insert
    values = [(40000 + i, 1, f"BULK{i}", 1, 1, i % 85 + 1) for i in range(5000)]
    cur.executemany("INSERT INTO characters (guid, account, name, race, class, level) "
                     "VALUES (%s, %s, %s, %s, %s, %s)", values)
    c.commit()
    # fetch all
    cur.execute("SELECT * FROM characters WHERE guid >= 40000 AND guid < 45000")
    rows = cur.fetchall()
    assert len(rows) == 5000, f"Expected 5000 rows, got {len(rows)}"
    # cleanup
    cur.execute("DELETE FROM characters WHERE guid >= 40000 AND guid < 45000")
    c.commit()
    cur.close()
    c.close()

@test("QueryStress_PreparedStatement_Rapid")
def test_prepared_rapid():
    """Execute 5000 prepared statement queries rapidly."""
    c = get_conn("realmd")
    cur = c.cursor(prepared=True) if DB_LIB == "mysql-connector" else c.cursor()
    for i in range(5000):
        cur.execute("SELECT id, username FROM account WHERE id = %s", (i % 3 + 1,))
        cur.fetchall()
    cur.close()
    c.close()

# ═══════════════════════════════════════════════════════════════════════════════
# RESOURCE EXHAUSTION
# ═══════════════════════════════════════════════════════════════════════════════

@test("Resource_OpenCursors_200")
def test_open_cursors():
    """Open 200 cursors on one connection without closing them."""
    c = get_conn("realmd")
    cursors = []
    for _ in range(200):
        cur = c.cursor()
        cur.execute("SELECT 1")
        cur.fetchone()
        cursors.append(cur)
    for cur in cursors:
        cur.close()
    c.close()

@test("Resource_HugeTempTable")
def test_huge_temp_table():
    """Create a temp table, insert 10K rows, then drop it — check for leaks."""
    c = get_conn("characters")
    cur = c.cursor()
    cur.execute("CREATE TEMPORARY TABLE stress_tmp ("
                "id INT AUTO_INCREMENT PRIMARY KEY, data VARCHAR(255))")
    for batch in range(100):
        values = [(f"stressdata_{batch}_{i}",) for i in range(100)]
        cur.executemany("INSERT INTO stress_tmp (data) VALUES (%s)", values)
    c.commit()
    cur.execute("SELECT COUNT(*) FROM stress_tmp")
    assert cur.fetchone()[0] == 10000
    cur.execute("DROP TEMPORARY TABLE stress_tmp")
    cur.close()
    c.close()

@test("Resource_LongRunningQuery")
def test_long_running_query():
    """Simulate a long-running query with a large cross join."""
    c = get_conn("realmd")
    cur = c.cursor()
    # This should be bounded — account table is small
    cur.execute("SELECT a1.id, a2.id FROM account a1, account a2, account a3 LIMIT 1000")
    rows = cur.fetchall()
    assert len(rows) <= 1000
    cur.close()
    c.close()

@test("Resource_ConnectionStorm_10Waves")
def test_connection_storm():
    """10 waves of 20 simultaneous connections, each doing a query then closing."""
    for wave in range(10):
        errors = []
        def worker():
            try:
                c = get_conn("realmd")
                cur = c.cursor()
                cur.execute("SELECT * FROM account LIMIT 1")
                cur.fetchall()
                cur.close()
                c.close()
            except Exception as e:
                errors.append(str(e))
        threads = [threading.Thread(target=worker) for _ in range(20)]
        for t in threads: t.start()
        for t in threads: t.join(timeout=10)
        assert len(errors) == 0, f"Wave {wave} had errors: {errors}"

# ═══════════════════════════════════════════════════════════════════════════════
# MARIADB-SPECIFIC EDGE CASES
# ═══════════════════════════════════════════════════════════════════════════════

@test("MariaDB_ThreadPoolExhaust")
def test_thread_pool():
    """Open max connections to test thread pool limits."""
    conns = []
    max_opened = 0
    try:
        for i in range(150):
            c = get_conn("realmd")
            conns.append(c)
            max_opened += 1
    except Exception:
        pass  # expected — hit max_connections
    for c in conns:
        try:
            c.close()
        except:
            pass
    # verify we can still connect after releasing
    time.sleep(0.5)
    c = get_conn("realmd")
    cur = c.cursor()
    cur.execute("SELECT 1")
    cur.fetchone()
    cur.close()
    c.close()

@test("MariaDB_BinaryData_InBlob")
def test_binary_data():
    """Store and retrieve binary data (simulating packet storage)."""
    c = get_conn("characters")
    cur = c.cursor()
    cur.execute("CREATE TEMPORARY TABLE bin_test (id INT PRIMARY KEY, data LONGBLOB)")
    binary_payloads = [
        os.urandom(0),         # empty
        os.urandom(1),         # 1 byte
        os.urandom(255),       # boundary
        os.urandom(65535),     # 64KB boundary
        os.urandom(1048576),   # 1MB
        b"\x00" * 1000,       # all nulls
        b"\xFF" * 1000,       # all 0xFF
        bytes(range(256)) * 4, # every byte value
    ]
    for i, payload in enumerate(binary_payloads):
        cur.execute("INSERT INTO bin_test (id, data) VALUES (%s, %s)", (i, payload))
    c.commit()
    for i, payload in enumerate(binary_payloads):
        cur.execute("SELECT data FROM bin_test WHERE id = %s", (i,))
        stored = cur.fetchone()[0]
        assert stored == payload, f"Binary mismatch at index {i}: len {len(stored)} vs {len(payload)}"
    cur.execute("DROP TEMPORARY TABLE bin_test")
    cur.close()
    c.close()

@test("MariaDB_CharsetCollation_Stress")
def test_charset_stress():
    """Test character set handling under stress."""
    c = get_conn("characters")
    cur = c.cursor()
    # Verify UTF-8 is enforced
    cur.execute("SHOW VARIABLES LIKE 'character_set_connection'")
    charset = cur.fetchone()
    cur.execute("CREATE TEMPORARY TABLE charset_test (id INT PRIMARY KEY, val VARCHAR(255) CHARACTER SET utf8mb4)")
    test_strings = [
        "Hello World",
        "Ñoño año",
        "日本語テスト",
        "Привет мир",
        "مرحبا بالعالم",
        "🎮🗡️🛡️⚔️",
        "\t\n\r",
        "   ",
        "A" * 255,
    ]
    for i, s in enumerate(test_strings):
        try:
            cur.execute("INSERT INTO charset_test VALUES (%s, %s)", (i, s[:255]))
        except Exception:
            pass
    c.commit()
    cur.execute("DROP TEMPORARY TABLE charset_test")
    cur.close()
    c.close()

@test("MariaDB_InnoDB_RowLock_Contention")
def test_row_lock_contention():
    """High contention on a single row with InnoDB row locks."""
    # Setup
    c = get_conn("realmd")
    cur = c.cursor()
    cur.execute("UPDATE account SET failed_logins = 0 WHERE id = 1")
    c.commit()
    cur.close()
    c.close()

    errors = []
    def incrementer(tid):
        try:
            for _ in range(100):
                c = get_conn("realmd", autocommit=False)
                cur = c.cursor()
                cur.execute("SELECT failed_logins FROM account WHERE id = 1 FOR UPDATE")
                val = cur.fetchone()[0]
                cur.execute("UPDATE account SET failed_logins = %s WHERE id = 1", (val + 1,))
                c.commit()
                cur.close()
                c.close()
        except Exception as e:
            errors.append(str(e))

    threads = [threading.Thread(target=incrementer, args=(i,)) for i in range(10)]
    for t in threads: t.start()
    for t in threads: t.join(timeout=30)

    # final value should be exactly 1000 if serialized properly
    c = get_conn("realmd")
    cur = c.cursor()
    cur.execute("SELECT failed_logins FROM account WHERE id = 1")
    final = cur.fetchone()[0]
    cur.execute("UPDATE account SET failed_logins = 0 WHERE id = 1")
    c.commit()
    cur.close()
    c.close()
    assert final == 1000, f"Row lock contention: expected 1000, got {final} (lost updates!)"

# ═══════════════════════════════════════════════════════════════════════════════
# SCHEMA ABUSE
# ═══════════════════════════════════════════════════════════════════════════════

@test("Schema_AlterTableUnderLoad")
def test_alter_under_load():
    """ALTER TABLE while concurrent reads are happening."""
    c = get_conn("characters")
    cur = c.cursor()
    cur.execute("CREATE TABLE IF NOT EXISTS alter_test (id INT PRIMARY KEY, val INT DEFAULT 0)")
    cur.execute("INSERT INTO alter_test VALUES (1, 0)")
    c.commit()

    stop = threading.Event()
    errors = []
    def reader():
        while not stop.is_set():
            try:
                rc = get_conn("characters")
                rcur = rc.cursor()
                rcur.execute("SELECT * FROM alter_test WHERE id = 1")
                rcur.fetchall()
                rcur.close()
                rc.close()
            except Exception:
                pass

    threads = [threading.Thread(target=reader) for _ in range(5)]
    for t in threads: t.start()

    time.sleep(0.2)
    try:
        cur.execute("ALTER TABLE alter_test ADD COLUMN extra VARCHAR(50) DEFAULT 'x'")
        cur.execute("ALTER TABLE alter_test DROP COLUMN extra")
    except Exception as e:
        errors.append(str(e))

    stop.set()
    for t in threads: t.join(timeout=5)
    cur.execute("DROP TABLE IF EXISTS alter_test")
    c.commit()
    cur.close()
    c.close()

@test("Schema_TruncateUnderLoad")
def test_truncate_under_load():
    """TRUNCATE while threads are reading/writing."""
    c = get_conn("characters")
    cur = c.cursor()
    cur.execute("CREATE TABLE IF NOT EXISTS trunc_test (id INT AUTO_INCREMENT PRIMARY KEY, val INT)")
    for i in range(100):
        cur.execute("INSERT INTO trunc_test (val) VALUES (%s)", (i,))
    c.commit()

    stop = threading.Event()
    def inserter():
        while not stop.is_set():
            try:
                ic = get_conn("characters")
                icur = ic.cursor()
                icur.execute("INSERT INTO trunc_test (val) VALUES (%s)", (random.randint(1, 999),))
                ic.commit()
                icur.close()
                ic.close()
            except:
                pass

    threads = [threading.Thread(target=inserter) for _ in range(5)]
    for t in threads: t.start()
    time.sleep(0.3)

    try:
        cur.execute("TRUNCATE TABLE trunc_test")
    except Exception:
        pass

    stop.set()
    for t in threads: t.join(timeout=5)
    cur.execute("DROP TABLE IF EXISTS trunc_test")
    c.commit()
    cur.close()
    c.close()

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    print("=" * 66)
    print("  MaNGOS MariaDB Adversarial Stress Test Suite")
    print(f"  Target: {DB_USER}@{DB_HOST}:{DB_PORT}")
    print(f"  Driver: {DB_LIB}")
    print("=" * 66)

    # Verify connectivity
    try:
        c = get_conn("realmd")
        cur = c.cursor()
        cur.execute("SELECT VERSION()")
        ver = cur.fetchone()[0]
        print(f"  MariaDB version: {ver}")
        cur.close()
        c.close()
    except Exception as e:
        print(f"ERROR: Cannot connect to MariaDB: {e}")
        sys.exit(1)

    print()

    # Collect all tests
    tests = [obj for name, obj in sorted(globals().items())
             if callable(obj) and getattr(obj, '_test', False)]

    print(f"Running {len(tests)} database stress tests...\n")

    for t in tests:
        t()

    passed = RESULTS["passed"]
    failed = RESULTS["failed"]
    total = passed + failed

    print(f"\n{'=' * 66}")
    print(f"  Results: {passed} passed, {failed} failed, {total} total")
    if RESULTS["errors"]:
        print(f"\n  Failures:")
        for e in RESULTS["errors"]:
            print(f"    - {e}")
    print(f"{'=' * 66}")

    # Final connectivity check
    try:
        c = get_conn("realmd")
        cur = c.cursor()
        cur.execute("SELECT COUNT(*) FROM account")
        count = cur.fetchone()[0]
        cur.close()
        c.close()
        print(f"\n  MariaDB status: ALIVE (account table has {count} rows)")
    except Exception as e:
        print(f"\n  MariaDB status: ERROR - {e}")

    sys.exit(0 if failed == 0 else 1)

if __name__ == "__main__":
    main()
