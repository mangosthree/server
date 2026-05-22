#!/usr/bin/env python3
"""
Adversarial stress test for MaNGOS server running in test mode.
Tries to crash the server via network attacks on port 8085.
"""

import socket
import struct
import time
import sys
import os
import random
import threading
import signal

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 8085
RESULTS = {"passed": 0, "failed": 0, "errors": []}

def check_server_alive():
    """Check if the server is still listening."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2)
        s.connect((SERVER_HOST, SERVER_PORT))
        s.close()
        return True
    except:
        return False

def test(name):
    """Decorator for test functions."""
    def decorator(func):
        def wrapper():
            try:
                func()
                RESULTS["passed"] += 1
                print(f"  [PASS] {name}")
            except AssertionError as e:
                RESULTS["failed"] += 1
                RESULTS["errors"].append(f"{name}: {e}")
                print(f"  [FAIL] {name}: {e}")
            except Exception as e:
                RESULTS["failed"] += 1
                RESULTS["errors"].append(f"{name}: {e}")
                print(f"  [ERROR] {name}: {e}")
        wrapper.__name__ = name
        return wrapper
    return decorator

def connect(timeout=3):
    """Create a connection to the server."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect((SERVER_HOST, SERVER_PORT))
    return s

def safe_send(sock, data):
    """Send data, ignoring broken pipe."""
    try:
        sock.sendall(data)
        return True
    except (BrokenPipeError, ConnectionResetError, OSError):
        return False

def safe_recv(sock, size=4096):
    """Receive data, ignoring errors."""
    try:
        return sock.recv(size)
    except (socket.timeout, ConnectionResetError, OSError):
        return b""

# ============================================================================
# Connection Stress Tests
# ============================================================================

@test("RapidConnectDisconnect")
def test_rapid_connect_disconnect():
    """Open and close connections as fast as possible."""
    for _ in range(100):
        try:
            s = connect(timeout=1)
            s.close()
        except:
            pass
    assert check_server_alive(), "Server died after rapid connect/disconnect"

@test("SimultaneousConnections")
def test_simultaneous_connections():
    """Open many connections at once."""
    sockets = []
    for _ in range(50):
        try:
            s = connect(timeout=2)
            sockets.append(s)
        except:
            break
    for s in sockets:
        try:
            s.close()
        except:
            pass
    time.sleep(0.5)
    assert check_server_alive(), "Server died after simultaneous connections"

@test("ConnectAndAbandon")
def test_connect_and_abandon():
    """Connect but never send anything, let it timeout."""
    sockets = []
    for _ in range(20):
        try:
            s = connect(timeout=1)
            sockets.append(s)
        except:
            break
    time.sleep(2)
    for s in sockets:
        try:
            s.close()
        except:
            pass
    assert check_server_alive(), "Server died after abandoned connections"

@test("HalfOpenConnections")
def test_half_open():
    """Connect, send partial data, then close."""
    for _ in range(30):
        try:
            s = connect(timeout=1)
            safe_send(s, b"\x00")
            s.close()
        except:
            pass
    assert check_server_alive(), "Server died after half-open connections"

# ============================================================================
# Malformed Packet Tests
# ============================================================================

@test("ZeroLengthPacket")
def test_zero_length_packet():
    """Send a packet with zero length."""
    s = connect()
    # WoW packet header: 2 bytes size + 4 bytes opcode
    safe_send(s, struct.pack(">H", 0) + struct.pack("<I", 0))
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on zero-length packet"

@test("MaxLengthPacket")
def test_max_length_packet():
    """Send a packet claiming maximum size."""
    s = connect()
    safe_send(s, struct.pack(">H", 0xFFFF) + struct.pack("<I", 0))
    safe_send(s, b"\x00" * 10000)
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on max-length packet"

@test("NegativeSizeHeader")
def test_negative_size_header():
    """Send packet with size bytes that could be interpreted as negative."""
    s = connect()
    safe_send(s, b"\xFF\xFF\xFF\xFF\xFF\xFF")
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on negative-size header"

@test("GarbageOpcodes")
def test_garbage_opcodes():
    """Send packets with random invalid opcodes."""
    s = connect()
    for _ in range(50):
        opcode = random.randint(0, 0xFFFF)
        size = random.randint(4, 20)
        pkt = struct.pack(">H", size) + struct.pack("<H", opcode)
        pkt += os.urandom(size)
        safe_send(s, pkt)
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on garbage opcodes"

@test("TruncatedHeader")
def test_truncated_header():
    """Send only 1 byte of the header."""
    s = connect()
    safe_send(s, b"\x00")
    time.sleep(1)
    s.close()
    assert check_server_alive(), "Server died on truncated header"

@test("OnlyHeaderNoBody")
def test_only_header():
    """Send a header claiming data follows, but send nothing."""
    s = connect()
    safe_send(s, struct.pack(">H", 100) + struct.pack("<H", 0x01))
    time.sleep(1)
    s.close()
    assert check_server_alive(), "Server died on header-only packet"

# ============================================================================
# Data Flood Tests
# ============================================================================

@test("BinaryFlood1MB")
def test_binary_flood():
    """Flood the connection with 1MB of random data."""
    s = connect()
    data = os.urandom(1024 * 1024)
    safe_send(s, data)
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on 1MB binary flood"

@test("NullByteFlood")
def test_null_flood():
    """Flood with null bytes."""
    s = connect()
    safe_send(s, b"\x00" * 100000)
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on null byte flood"

@test("PatternFlood0xFF")
def test_ff_flood():
    """Flood with 0xFF bytes."""
    s = connect()
    safe_send(s, b"\xFF" * 100000)
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on 0xFF flood"

@test("RepeatedSmallPackets")
def test_repeated_small_packets():
    """Send thousands of tiny packets rapidly."""
    s = connect()
    for _ in range(5000):
        if not safe_send(s, b"\x00\x04\x00\x00"):
            break
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on rapid small packets"

# ============================================================================
# WoW Protocol Attacks
# ============================================================================

@test("FakeAuthChallenge")
def test_fake_auth_challenge():
    """Send a fake CMSG_AUTH_SESSION packet."""
    s = connect()
    # First receive the server's auth challenge
    data = safe_recv(s, 4096)
    # Send a fake auth session (opcode 0x1ED for Cata)
    auth_pkt = struct.pack(">H", 200) + struct.pack("<H", 0x1ED)
    auth_pkt += b"\x00" * 196
    safe_send(s, auth_pkt)
    time.sleep(1)
    s.close()
    assert check_server_alive(), "Server died on fake auth challenge"

@test("WrongBuildNumber")
def test_wrong_build():
    """Send auth with wrong client build."""
    s = connect()
    data = safe_recv(s, 4096)
    # Craft auth session with bogus build
    pkt = struct.pack(">H", 50)
    pkt += struct.pack("<H", 0x1ED)  # CMSG_AUTH_SESSION
    pkt += struct.pack("<I", 99999)   # bogus build
    pkt += struct.pack("<I", 0)       # login server id
    pkt += b"AAAA\x00"               # account name
    pkt += b"\x00" * 35
    safe_send(s, pkt)
    time.sleep(1)
    s.close()
    assert check_server_alive(), "Server died on wrong build number"

@test("OversizedAccountName")
def test_oversized_account_name():
    """Send auth with extremely long account name."""
    s = connect()
    data = safe_recv(s, 4096)
    name = b"A" * 10000 + b"\x00"
    pkt = struct.pack(">H", len(name) + 10)
    pkt += struct.pack("<H", 0x1ED)
    pkt += struct.pack("<I", 15595)
    pkt += struct.pack("<I", 0)
    pkt += name
    safe_send(s, pkt)
    time.sleep(1)
    s.close()
    assert check_server_alive(), "Server died on oversized account name"

@test("NullAccountName")
def test_null_account_name():
    """Send auth with only null terminators."""
    s = connect()
    data = safe_recv(s, 4096)
    pkt = struct.pack(">H", 20)
    pkt += struct.pack("<H", 0x1ED)
    pkt += struct.pack("<I", 15595)
    pkt += struct.pack("<I", 0)
    pkt += b"\x00" * 12
    safe_send(s, pkt)
    time.sleep(1)
    s.close()
    assert check_server_alive(), "Server died on null account name"

@test("RepeatedAuthAttempts")
def test_repeated_auth():
    """Send many auth attempts on one connection."""
    s = connect()
    data = safe_recv(s, 4096)
    for _ in range(100):
        pkt = struct.pack(">H", 20)
        pkt += struct.pack("<H", 0x1ED)
        pkt += struct.pack("<I", 15595)
        pkt += struct.pack("<I", 0)
        pkt += b"\x00" * 12
        if not safe_send(s, pkt):
            break
    time.sleep(1)
    s.close()
    assert check_server_alive(), "Server died on repeated auth"

# ============================================================================
# Timing / Race Condition Tests
# ============================================================================

@test("ConcurrentFlood")
def test_concurrent_flood():
    """Multiple threads flooding simultaneously."""
    errors = []
    def flood_worker():
        try:
            s = connect(timeout=2)
            for _ in range(100):
                if not safe_send(s, os.urandom(random.randint(1, 500))):
                    break
            s.close()
        except Exception as e:
            errors.append(str(e))

    threads = [threading.Thread(target=flood_worker) for _ in range(20)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=10)

    time.sleep(1)
    assert check_server_alive(), f"Server died on concurrent flood"

@test("ConnectDuringFlood")
def test_connect_during_flood():
    """Try to connect while another thread is flooding."""
    stop = threading.Event()
    def flooder():
        while not stop.is_set():
            try:
                s = connect(timeout=1)
                safe_send(s, os.urandom(1000))
                s.close()
            except:
                pass

    threads = [threading.Thread(target=flooder) for _ in range(5)]
    for t in threads:
        t.start()

    time.sleep(2)
    success = check_server_alive()
    stop.set()
    for t in threads:
        t.join(timeout=5)
    assert success, "Server died during flood + connect test"

@test("RapidReconnect")
def test_rapid_reconnect():
    """Rapidly disconnect and reconnect."""
    for _ in range(50):
        try:
            s = connect(timeout=1)
            data = safe_recv(s, 4096)
            safe_send(s, os.urandom(10))
            s.shutdown(socket.SHUT_RDWR)
            s.close()
        except:
            pass
    assert check_server_alive(), "Server died on rapid reconnect"

# ============================================================================
# Edge Case Packets
# ============================================================================

@test("PacketSizeOverflow")
def test_packet_size_overflow():
    """Send packet where size field overflows uint16."""
    s = connect()
    # 3-byte header size (big packet)
    safe_send(s, b"\x80\x00\x00" + struct.pack("<H", 0))
    safe_send(s, os.urandom(100))
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on size overflow"

@test("AllValidOpcodes")
def test_all_valid_opcodes():
    """Iterate through opcode ranges sending minimal packets."""
    s = connect()
    data = safe_recv(s, 4096)
    for opcode in range(0, 0x600, 0x10):
        pkt = struct.pack(">H", 4) + struct.pack("<H", opcode) + b"\x00\x00"
        if not safe_send(s, pkt):
            break
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died iterating opcodes"

@test("InterleavedValidInvalid")
def test_interleaved():
    """Alternate between valid-looking and garbage packets."""
    s = connect()
    data = safe_recv(s, 4096)
    for i in range(100):
        if i % 2 == 0:
            pkt = struct.pack(">H", 4) + struct.pack("<H", 0x01) + b"\x00\x00"
        else:
            pkt = os.urandom(random.randint(1, 200))
        if not safe_send(s, pkt):
            break
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on interleaved packets"

@test("SlowlorisAttack")
def test_slowloris():
    """Send data one byte at a time very slowly."""
    s = connect(timeout=10)
    for i in range(20):
        if not safe_send(s, bytes([i % 256])):
            break
        time.sleep(0.1)
    s.close()
    assert check_server_alive(), "Server died on slowloris"

@test("UDPOnTCPPort")
def test_udp_probe():
    """Send UDP packets to the TCP port (should be ignored)."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(1)
    for _ in range(10):
        try:
            s.sendto(os.urandom(100), (SERVER_HOST, SERVER_PORT))
        except:
            pass
    s.close()
    assert check_server_alive(), "Server died on UDP probes"

# ============================================================================
# Stress Sequences
# ============================================================================

@test("ConnectReadClose1000x")
def test_connect_read_close_1000():
    """Connect, read server greeting, close - 1000 times."""
    for _ in range(1000):
        try:
            s = connect(timeout=1)
            data = safe_recv(s, 4096)
            s.close()
        except:
            pass
    assert check_server_alive(), "Server died after 1000 connect-read-close cycles"

@test("MixedSizeFlood")
def test_mixed_size_flood():
    """Send packets of wildly varying sizes."""
    s = connect()
    sizes = [0, 1, 2, 3, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 4096, 8192, 16384, 32768, 65535]
    for size in sizes:
        pkt = struct.pack(">H", min(size, 0xFFFF))
        if size > 0:
            pkt += os.urandom(min(size, 8192))
        if not safe_send(s, pkt):
            break
    time.sleep(0.5)
    s.close()
    assert check_server_alive(), "Server died on mixed-size flood"

@test("FinalStressTest")
def test_final_stress():
    """Combined stress: 10 threads each doing 50 connect-flood-close."""
    errors = []
    def worker():
        for _ in range(50):
            try:
                s = connect(timeout=2)
                data = safe_recv(s, 4096)
                safe_send(s, os.urandom(random.randint(10, 2000)))
                s.close()
            except:
                pass

    threads = [threading.Thread(target=worker) for _ in range(10)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=30)

    time.sleep(2)
    assert check_server_alive(), "Server died on final stress test"


# ============================================================================
# Main
# ============================================================================

def main():
    print("=" * 60)
    print("MaNGOS Server Adversarial Network Stress Test")
    print("=" * 60)

    if not check_server_alive():
        print(f"ERROR: Server not responding on {SERVER_HOST}:{SERVER_PORT}")
        sys.exit(1)

    print(f"Server is alive on {SERVER_HOST}:{SERVER_PORT}\n")

    # Collect all test functions
    tests = [v for v in globals().values() if callable(v) and hasattr(v, '__name__') and v.__name__.startswith(("test_", "Test"))]
    # Actually, collect by looking at decorated names
    tests = []
    for name, obj in list(globals().items()):
        if name.startswith("test_") and callable(obj):
            tests.append(obj)

    print(f"Running {len(tests)} stress tests...\n")

    for t in tests:
        t()
        # Quick check server is still up between tests
        if not check_server_alive():
            print(f"\n*** SERVER CRASHED after test: {t.__name__} ***")
            break

    print(f"\n{'=' * 60}")
    print(f"Results: {RESULTS['passed']} passed, {RESULTS['failed']} failed")
    if RESULTS['errors']:
        print(f"\nFailures:")
        for e in RESULTS['errors']:
            print(f"  - {e}")
    print(f"{'=' * 60}")

    # Final server status
    alive = check_server_alive()
    print(f"\nServer status: {'ALIVE' if alive else 'DEAD'}")

    sys.exit(0 if RESULTS['failed'] == 0 and alive else 1)

if __name__ == "__main__":
    main()
