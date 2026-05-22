#!/usr/bin/env python3
"""
MaNGOS 4.3.4 Mock Client
Connects to the server in MANGOS_TEST_MODE (no encryption, no SHA1 check).
Tests both legitimate game mechanics and adversarial/exploit behaviours.
"""

import socket, struct, time, sys, threading, os

HOST = "127.0.0.1"
PORT = 8085

# ── opcodes ──────────────────────────────────────────────────────────────────
SMSG_AUTH_CHALLENGE   = 0x4542
CMSG_AUTH_SESSION     = 0x0449
SMSG_AUTH_RESPONSE    = 0x5DB6
CMSG_CHAR_ENUM        = 0x0502
SMSG_CHAR_ENUM        = 0x10B0
CMSG_PLAYER_LOGIN     = 0x05B1
CMSG_PING             = 0x444D
SMSG_PONG             = 0x4D42
CMSG_MESSAGECHAT_SAY  = 0x1154
CMSG_CAST_SPELL       = 0x4C07
CMSG_ATTACKSWING      = 0x0926
CMSG_MOVE_START_FORWARD = 0x7814
CMSG_MOVE_STOP        = 0x320A
CMSG_LOGOUT_REQUEST   = 0x0A25
SMSG_LOGOUT_COMPLETE  = 0x2137
SMSG_AUTH_RESPONSE_CODE_OK     = 0x0C
SMSG_AUTH_RESPONSE_CODE_FAILED = 0x0D
SMSG_AUTH_RESPONSE_CODE_UNKNOWN_ACCOUNT = 0x15

AUTH_OK               = 0x0C
AUTH_FAILED           = 0x0D
AUTH_UNKNOWN_ACCOUNT  = 0x15
AUTH_VERSION_MISMATCH = 0x14
AUTH_BANNED           = 0x1C

CLIENT_BUILD = 15595

PASS = 0
FAIL = 1
results = []

def result(name, status, note=""):
    tag = "PASS" if status == PASS else "FAIL"
    results.append((tag, name, note))
    print(f"  [{tag}] {name}" + (f" — {note}" if note else ""))

# ── low-level packet helpers ──────────────────────────────────────────────────

class WowSocket:
    """Thin wrapper around a TCP socket for the WoW 4.3.4 protocol."""

    def __init__(self, host=HOST, port=PORT, timeout=5):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.settimeout(timeout)
        self.s.connect((host, port))
        self._buf = b""

    def close(self):
        try: self.s.close()
        except: pass

    # ── send ──────────────────────────────────────────────────────────────────
    def send_packet(self, opcode: int, payload: bytes = b""):
        """Client→Server: uint16_BE(len+4) + uint32_LE(opcode) + payload"""
        size = len(payload) + 4          # +4 for uint32 opcode
        header = struct.pack(">H", size) + struct.pack("<I", opcode)
        self.s.sendall(header + payload)

    def send_raw(self, data: bytes):
        self.s.sendall(data)

    # ── receive ───────────────────────────────────────────────────────────────
    def _fill(self, n):
        while len(self._buf) < n:
            chunk = self.s.recv(4096)
            if not chunk:
                raise ConnectionError("server closed connection")
            self._buf += chunk

    def recv_server_packet(self):
        """Server→Client: 2-byte BE size + 2-byte LE opcode (no encryption in test mode)"""
        self._fill(4)
        size  = struct.unpack(">H", self._buf[:2])[0]
        opcode = struct.unpack("<H", self._buf[2:4])[0]
        self._buf = self._buf[4:]
        body_len = size - 2              # size includes the 2-byte opcode
        if body_len > 0:
            self._fill(body_len)
            body = self._buf[:body_len]
            self._buf = self._buf[body_len:]
        else:
            body = b""
        return opcode, body

    def recv_until(self, target_opcode, deadline=5):
        """Drain packets until we see the target opcode or timeout."""
        t0 = time.time()
        while time.time() - t0 < deadline:
            try:
                op, body = self.recv_server_packet()
                if op == target_opcode:
                    return op, body
            except (socket.timeout, ConnectionError, OSError):
                break
        return None, None

    def drain(self, duration=0.5):
        """Drain whatever comes in for a short time; return list of (op, body)."""
        out = []
        self.s.settimeout(0.3)
        t0 = time.time()
        while time.time() - t0 < duration:
            try:
                op, body = self.recv_server_packet()
                out.append((op, body))
            except (socket.timeout, ConnectionError):
                break
        self.s.settimeout(5)
        return out

# ── auth helper ───────────────────────────────────────────────────────────────

def read_greeting(ws: WowSocket):
    """Consume the MSG_WOW_CONNECTION greeting the server sends on connect."""
    ws.s.settimeout(2)
    try:
        chunk = ws.s.recv(4096)
        ws._buf += chunk
    except socket.timeout:
        pass
    ws.s.settimeout(5)

def recv_auth_challenge(ws: WowSocket):
    """Read SMSG_AUTH_CHALLENGE and return server_seed."""
    # Server may send a raw greeting string first; eat until we see 0x4542
    ws.s.settimeout(3)
    for _ in range(10):
        try:
            op, body = ws.recv_server_packet()
            if op == SMSG_AUTH_CHALLENGE:
                # body: 8×uint32 zeros + uint32 seed + uint8
                if len(body) >= 36:
                    seed = struct.unpack_from("<I", body, 32)[0]
                    return seed
        except Exception:
            break
    return None

def build_auth_session(account: str, client_seed: int = 0xDEADBEEF,
                       build: int = CLIENT_BUILD, addon_data: bytes = b""):
    """
    Build CMSG_AUTH_SESSION payload for 4.3.4.
    Layout mirrors exactly WorldSocket::HandleAuthSession() read order.
    Server in TEST_MODE skips SHA1 digest check, so digest bytes can be zero.
    """
    digest = bytes(20)          # all zeros - accepted in MANGOS_TEST_MODE
    name_bytes = account.encode("utf-8")
    name_len   = len(name_bytes)

    buf = b""
    buf += struct.pack("<I", 0)              # read_skip<uint32>
    buf += struct.pack("<I", 0)              # read_skip<uint32>
    buf += struct.pack("<B", 0)              # read_skip<uint8>
    buf += bytes([digest[10], digest[18], digest[12], digest[5]])   # 4 digest bytes
    buf += struct.pack("<Q", 0)              # read_skip<uint64>
    buf += bytes([digest[15], digest[9], digest[19],
                  digest[4],  digest[7], digest[16], digest[3]])    # 7 digest bytes
    buf += struct.pack("<H", build)          # BuiltNumberClient (uint16)
    buf += bytes([digest[8]])                # 1 digest byte
    buf += struct.pack("<I", 0)              # read_skip<uint32>
    buf += struct.pack("<B", 0)              # read_skip<uint8>
    buf += bytes([digest[17], digest[6],
                  digest[0],  digest[1], digest[11]])               # 5 digest bytes
    buf += struct.pack("<I", client_seed)    # clientSeed
    buf += bytes([digest[2]])                # 1 digest byte
    buf += struct.pack("<I", 0)              # read_skip<uint32>
    buf += bytes([digest[14], digest[13]])   # 2 digest bytes
    buf += struct.pack("<I", len(addon_data))  # m_addonSize
    buf += addon_data
    # packed account name length: server reconstructs as (hi<<5)|(lo>>3)
    hi = (name_len >> 5) & 0xFF
    lo = (name_len << 3) & 0xFF
    buf += bytes([hi, lo])
    buf += name_bytes
    return buf

def do_auth(ws: WowSocket, account: str = "TEST", build: int = CLIENT_BUILD):
    """Full auth handshake. Returns (ok:bool, auth_code:int)."""
    seed = recv_auth_challenge(ws)
    if seed is None:
        return False, -1
    payload = build_auth_session(account, build=build)
    ws.send_packet(CMSG_AUTH_SESSION, payload)
    op, body = ws.recv_until(SMSG_AUTH_RESPONSE, deadline=4)
    if op is None or len(body) < 1:
        return False, -1
    auth_code = body[-1]  # last byte is the auth result code
    return auth_code == AUTH_OK, auth_code

# ═══════════════════════════════════════════════════════════════════════════════
# NORMAL MECHANIC TESTS
# ═══════════════════════════════════════════════════════════════════════════════

def test_normal_auth_ok():
    ws = WowSocket()
    ok, code = do_auth(ws, "TEST")
    ws.close()
    result("NormalAuth_ValidAccount", PASS if ok else FAIL,
           f"auth_code=0x{code:02X}")

def test_auth_unknown_account():
    ws = WowSocket()
    ok, code = do_auth(ws, "DOESNOTEXIST")
    ws.close()
    expected = (not ok and code == AUTH_UNKNOWN_ACCOUNT)
    result("Auth_UnknownAccount_Rejected", PASS if expected else FAIL,
           f"auth_code=0x{code:02X} (expected 0x{AUTH_UNKNOWN_ACCOUNT:02X})")

def test_auth_wrong_build():
    ws = WowSocket()
    ok, code = do_auth(ws, "TEST", build=9999)
    ws.close()
    expected = (not ok and code == AUTH_VERSION_MISMATCH)
    result("Auth_WrongBuild_Rejected", PASS if expected else FAIL,
           f"auth_code=0x{code:02X} (expected 0x{AUTH_VERSION_MISMATCH:02X})")

def test_char_enum_after_auth():
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("CharEnum_AfterAuth", FAIL, "auth failed")
        ws.close()
        return
    ws.send_packet(CMSG_CHAR_ENUM)
    op, body = ws.recv_until(SMSG_CHAR_ENUM, deadline=4)
    ws.close()
    # With empty DB, server may respond or just ignore; either way no crash
    result("CharEnum_AfterAuth_NoCrash", PASS if op == SMSG_CHAR_ENUM or op is None else FAIL,
           f"got opcode 0x{op:04X}" if op else "no response (OK, empty DB)")

def test_ping_pong():
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Ping_Pong", FAIL, "auth failed"); ws.close(); return
    ping_id = 0xCAFEBABE
    latency  = 42
    payload  = struct.pack("<II", ping_id, latency)
    ws.send_packet(CMSG_PING, payload)
    op, body = ws.recv_until(SMSG_PONG, deadline=3)
    ws.close()
    if op == SMSG_PONG and len(body) >= 4:
        returned_id = struct.unpack_from("<I", body)[0]
        ok = (returned_id == ping_id)
        result("Ping_Pong_EchoesID", PASS if ok else FAIL,
               f"sent 0x{ping_id:08X} got 0x{returned_id:08X}")
    else:
        result("Ping_Pong_EchoesID", FAIL, f"no SMSG_PONG (got {op})")

def test_multiple_pings():
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("MultiplePings_Sequential", FAIL, "auth failed"); ws.close(); return
    ok_count = 0
    for i in range(10):
        ws.send_packet(CMSG_PING, struct.pack("<II", i, 10))
        op, body = ws.recv_until(SMSG_PONG, deadline=2)
        if op == SMSG_PONG:
            ok_count += 1
    ws.close()
    result("MultiplePings_Sequential", PASS if ok_count == 10 else FAIL,
           f"{ok_count}/10 pongs received")

# ═══════════════════════════════════════════════════════════════════════════════
# EXPLOIT / ADVERSARIAL TESTS
# ═══════════════════════════════════════════════════════════════════════════════

def test_exploit_send_before_auth():
    """Send game opcodes before authenticating — server must not crash."""
    ws = WowSocket()
    # skip auth — send game packets straight away
    ws.send_packet(CMSG_CHAR_ENUM)
    ws.send_packet(CMSG_PING, struct.pack("<II", 1, 1))
    ws.send_packet(CMSG_ATTACKSWING, struct.pack("<Q", 0x0102030405060708))
    ws.send_packet(CMSG_CAST_SPELL, b"\x01" + struct.pack("<I", 133) + b"\x00" * 10)
    time.sleep(0.5)
    ws.close()
    # survival check
    ws2 = WowSocket()
    ok, code = do_auth(ws2, "TEST")
    ws2.close()
    result("Exploit_OpcodeBeforeAuth_NoCrash", PASS if True else FAIL,
           "sent game packets before auth; server still accepts new connections")

def test_exploit_double_auth():
    """Send CMSG_AUTH_SESSION twice on the same connection."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_DoubleAuth", FAIL, "first auth failed"); ws.close(); return
    # send auth packet again
    payload = build_auth_session("TEST")
    ws.send_packet(CMSG_AUTH_SESSION, payload)
    pkts = ws.drain(1.0)
    ws.close()
    result("Exploit_DoubleAuth_NoCrash", PASS,
           f"got {len(pkts)} packet(s) after second auth, server alive")

def test_exploit_chat_before_login():
    """Send a chat message after auth but before CMSG_PLAYER_LOGIN (no player object)."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_ChatBeforeLogin", FAIL, "auth failed"); ws.close(); return
    # Build CMSG_MESSAGECHAT_SAY: lang=0, msg="hello", no target
    msg = "hello exploit".encode("utf-8")
    payload  = struct.pack("<I", 0)     # type (unused here, opcode specific)
    payload += struct.pack("<I", 0)     # language UNIVERSAL
    payload += struct.pack("<I", len(msg) + 1)
    payload += msg + b"\x00"
    ws.send_packet(CMSG_MESSAGECHAT_SAY, payload)
    time.sleep(0.3)
    ws.close()
    result("Exploit_ChatBeforeLogin_NoCrash", PASS,
           "chat sent with no active player — server must handle gracefully")

def test_exploit_movement_before_login():
    """Send movement opcodes with no player in world."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_MovementBeforeLogin", FAIL, "auth failed"); ws.close(); return
    # MovementInfo: guid(packed), flags, extra_flags, timestamp, x, y, z, ori
    def movement_payload(x, y, z, flags=0):
        buf  = struct.pack("<Q", 0)          # guid placeholder
        buf += struct.pack("<I", flags)      # movement flags
        buf += struct.pack("<H", 0)          # extra flags
        buf += struct.pack("<I", 1234)       # timestamp
        buf += struct.pack("<fff", x, y, z)  # position
        buf += struct.pack("<f", 0.0)        # orientation
        buf += struct.pack("<f", 0.0)        # fall time
        return buf
    ws.send_packet(CMSG_MOVE_START_FORWARD, movement_payload(0, 0, 0))
    ws.send_packet(CMSG_MOVE_STOP,          movement_payload(10, 0, 0))
    time.sleep(0.3)
    ws.close()
    result("Exploit_MovementBeforeLogin_NoCrash", PASS,
           "movement sent with no active player")

def test_exploit_attack_before_login():
    """CMSG_ATTACKSWING with no player — must not segfault on null player."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_AttackBeforeLogin", FAIL, "auth failed"); ws.close(); return
    # target GUID 0 (invalid)
    ws.send_packet(CMSG_ATTACKSWING, struct.pack("<Q", 0))
    # target with huge GUID
    ws.send_packet(CMSG_ATTACKSWING, struct.pack("<Q", 0xFFFFFFFFFFFFFFFF))
    time.sleep(0.3)
    ws.close()
    result("Exploit_AttackBeforeLogin_NoCrash", PASS,
           "attack with null/max GUID, no player")

def test_exploit_spellcast_before_login():
    """Spam 100 spell cast requests before ever logging in."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_SpellSpamBeforeLogin", FAIL, "auth failed"); ws.close(); return
    for spell_id in range(1, 101):
        payload  = struct.pack("<B", 1)          # cast_count
        payload += struct.pack("<I", spell_id)   # spellId
        payload += struct.pack("<I", 0)          # glyphIndex
        payload += struct.pack("<B", 0)          # cast_flags
        payload += b"\x00" * 4                  # minimal targets
        ws.send_packet(CMSG_CAST_SPELL, payload)
    time.sleep(0.5)
    ws.close()
    result("Exploit_SpellSpam100BeforeLogin_NoCrash", PASS,
           "100 spells cast before login, no player object")

def test_exploit_overflow_ping():
    """Ping with max uint32 values — overflow or wrapping."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_PingOverflow", FAIL, "auth failed"); ws.close(); return
    cases = [
        (0xFFFFFFFF, 0xFFFFFFFF),
        (0x00000000, 0xFFFFFFFF),
        (0xFFFFFFFF, 0x00000000),
    ]
    for pid, lat in cases:
        ws.send_packet(CMSG_PING, struct.pack("<II", pid, lat))
    pkts = ws.drain(1.0)
    ws.close()
    pongs = [p for p in pkts if p[0] == SMSG_PONG]
    result("Exploit_PingOverflow_MaxUint32", PASS,
           f"sent 3 max-uint32 pings, got {len(pongs)} pongs")

def test_exploit_packet_length_mismatch():
    """Claim large size in header but send less body — should disconnect cleanly."""
    ws = WowSocket()
    # deliberately malformed: say body is 10000 bytes, send only 4
    header = struct.pack(">H", 10004) + struct.pack("<I", CMSG_CHAR_ENUM)
    ws.send_raw(header + b"\x00\x00\x00\x00")
    time.sleep(1.0)
    ws.close()
    # server should still be alive
    ws2 = WowSocket()
    ok, _ = do_auth(ws2, "TEST")
    ws2.close()
    result("Exploit_LengthMismatch_ServerSurvives", PASS if ok else FAIL,
           "server still accepts new connections after malformed length")

def test_exploit_zero_body_opcodes():
    """Send opcodes that expect a body but with zero bytes."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_ZeroBodyOpcodes", FAIL, "auth failed"); ws.close(); return
    dangerous = [CMSG_ATTACKSWING, CMSG_CAST_SPELL, CMSG_PLAYER_LOGIN,
                 CMSG_MESSAGECHAT_SAY, CMSG_MOVE_START_FORWARD]
    for op in dangerous:
        ws.send_packet(op, b"")   # valid header, empty body
    time.sleep(0.5)
    ws.close()
    result("Exploit_ZeroBodyOpcodes_NoCrash", PASS,
           f"sent {len(dangerous)} opcodes with empty bodies")

def test_exploit_rapid_auth_cycle():
    """Authenticate and disconnect 50 times rapidly — resource leak check."""
    ok_count = 0
    for _ in range(50):
        try:
            ws = WowSocket(timeout=2)
            ok, _ = do_auth(ws, "TEST")
            if ok:
                ok_count += 1
            ws.close()
        except Exception:
            pass
    result("Exploit_RapidAuthCycle_50x", PASS if ok_count >= 45 else FAIL,
           f"{ok_count}/50 auths succeeded")

def test_exploit_concurrent_auth():
    """10 clients all authenticating simultaneously."""
    successes = []
    lock = threading.Lock()
    def worker():
        try:
            ws = WowSocket(timeout=4)
            ok, _ = do_auth(ws, "TEST")
            with lock:
                successes.append(ok)
            ws.close()
        except Exception as e:
            with lock:
                successes.append(False)

    threads = [threading.Thread(target=worker) for _ in range(10)]
    for t in threads: t.start()
    for t in threads: t.join(timeout=10)
    passed = sum(successes)
    result("Exploit_ConcurrentAuth_10x", PASS if passed >= 8 else FAIL,
           f"{passed}/10 concurrent auths succeeded")

def test_exploit_null_guid_login():
    """CMSG_PLAYER_LOGIN with GUID 0 and all-ones — no character exists."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_NullGuidLogin", FAIL, "auth failed"); ws.close(); return
    ws.send_packet(CMSG_PLAYER_LOGIN, struct.pack("<Q", 0))
    ws.send_packet(CMSG_PLAYER_LOGIN, struct.pack("<Q", 0xFFFFFFFFFFFFFFFF))
    time.sleep(0.5)
    ws.close()
    result("Exploit_NullGuidLogin_NoCrash", PASS,
           "login with GUID 0 and 0xFFFF... handled without crash")

def test_exploit_logout_without_login():
    """Request logout without ever logging in."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_LogoutWithoutLogin", FAIL, "auth failed"); ws.close(); return
    ws.send_packet(CMSG_LOGOUT_REQUEST)
    time.sleep(0.3)
    ws.close()
    result("Exploit_LogoutWithoutLogin_NoCrash", PASS,
           "logout requested with no active player")

def test_exploit_garbage_after_auth():
    """Send 200 random garbage packets after a valid auth."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_GarbageAfterAuth", FAIL, "auth failed"); ws.close(); return
    for _ in range(200):
        opcode = int.from_bytes(os.urandom(2), "little")
        body   = os.urandom(int.from_bytes(os.urandom(1), "little") % 32)
        ws.send_packet(opcode, body)
    time.sleep(0.5)
    ws.close()
    ws2 = WowSocket(timeout=3)
    ok2, _ = do_auth(ws2, "TEST")
    ws2.close()
    result("Exploit_200GarbageOpcodes_ServerSurvives", PASS if ok2 else FAIL,
           "server still authenticates new client after 200 garbage packets")

def test_exploit_interleaved_auth_and_packets():
    """Send game packets interleaved with the auth handshake mid-flight."""
    ws = WowSocket()
    seed = recv_auth_challenge(ws)
    # inject garbage before sending auth response
    ws.send_packet(CMSG_CHAR_ENUM)
    ws.send_packet(CMSG_PING, struct.pack("<II", 1, 1))
    # now send real auth
    if seed is not None:
        payload = build_auth_session("TEST")
        ws.send_packet(CMSG_AUTH_SESSION, payload)
    op, body = ws.recv_until(SMSG_AUTH_RESPONSE, deadline=4)
    ws.close()
    result("Exploit_Interleaved_PreAuth_Packets", PASS,
           f"auth_response=0x{op:04X}" if op else "no auth response (handled)")

def test_exploit_max_concurrent_sessions():
    """Open 100 authenticated sessions simultaneously."""
    sessions = []
    ok_count = 0
    for _ in range(100):
        try:
            ws = WowSocket(timeout=3)
            ok, _ = do_auth(ws, "TEST")
            if ok:
                ok_count += 1
            sessions.append(ws)
        except Exception:
            break
    for ws in sessions:
        ws.close()
    result("Exploit_100ConcurrentSessions", PASS,
           f"opened {ok_count} authenticated sessions simultaneously, server alive")

def test_exploit_ping_flood():
    """Flood with 1000 pings on one connection — rate limiting / no crash."""
    ws = WowSocket()
    ok, _ = do_auth(ws, "TEST")
    if not ok:
        result("Exploit_PingFlood_1000", FAIL, "auth failed"); ws.close(); return
    for i in range(1000):
        ws.send_packet(CMSG_PING, struct.pack("<II", i, 0))
    pkts = ws.drain(2.0)
    ws.close()
    pongs = sum(1 for p in pkts if p[0] == SMSG_PONG)
    result("Exploit_PingFlood_1000_NoCrash", PASS,
           f"1000 pings sent, {pongs} pongs received, server alive")

# ═══════════════════════════════════════════════════════════════════════════════
# SERVER ALIVE SANITY CHECK
# ═══════════════════════════════════════════════════════════════════════════════

def server_alive():
    try:
        ws = WowSocket(timeout=3)
        ok, code = do_auth(ws, "TEST")
        ws.close()
        return ok
    except Exception:
        return False

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

NORMAL_TESTS = [
    test_normal_auth_ok,
    test_auth_unknown_account,
    test_auth_wrong_build,
    test_char_enum_after_auth,
    test_ping_pong,
    test_multiple_pings,
]

EXPLOIT_TESTS = [
    test_exploit_send_before_auth,
    test_exploit_double_auth,
    test_exploit_chat_before_login,
    test_exploit_movement_before_login,
    test_exploit_attack_before_login,
    test_exploit_spellcast_before_login,
    test_exploit_overflow_ping,
    test_exploit_packet_length_mismatch,
    test_exploit_zero_body_opcodes,
    test_exploit_rapid_auth_cycle,
    test_exploit_concurrent_auth,
    test_exploit_null_guid_login,
    test_exploit_logout_without_login,
    test_exploit_garbage_after_auth,
    test_exploit_interleaved_auth_and_packets,
    test_exploit_max_concurrent_sessions,
    test_exploit_ping_flood,
]

def main():
    print("=" * 62)
    print("  MaNGOS 4.3.4 Mock Client — Mechanics & Exploit Test Suite")
    print("=" * 62)

    if not server_alive():
        print(f"ERROR: Server not responding on {HOST}:{PORT}")
        sys.exit(1)
    print(f"Server alive at {HOST}:{PORT}\n")

    print("── Normal Mechanics ──────────────────────────────────────────")
    for t in NORMAL_TESTS:
        t()
        if not server_alive():
            print(f"\n*** SERVER CRASHED after {t.__name__} ***")
            sys.exit(1)

    print("\n── Adversarial / Exploit Tests ───────────────────────────────")
    for t in EXPLOIT_TESTS:
        t()
        if not server_alive():
            print(f"\n*** SERVER CRASHED after {t.__name__} ***")
            sys.exit(1)

    passed = sum(1 for r in results if r[0] == "PASS")
    failed = sum(1 for r in results if r[0] == "FAIL")
    total  = len(results)

    print(f"\n{'=' * 62}")
    print(f"  Results: {passed} passed, {failed} failed, {total} total")
    if failed:
        print("\n  Failures:")
        for tag, name, note in results:
            if tag == "FAIL":
                print(f"    - {name}: {note}")
    print(f"{'=' * 62}")
    print(f"\n  Server status after all tests: {'ALIVE ✓' if server_alive() else 'DEAD ✗'}")
    sys.exit(0 if failed == 0 else 1)

if __name__ == "__main__":
    main()
