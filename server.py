#!/usr/bin/env python3
"""
Thread Management Library — Live Bridge Server
Connects C binary output to browser GUI via WebSocket.
Streams real-time output and classifies log lines for color coding.

Usage:
    python3 server.py
Then open browser: http://localhost:8080
"""

import asyncio
import websockets
import threading
import json
import os
from http.server import HTTPServer, SimpleHTTPRequestHandler
from pathlib import Path

# ── Config ──
C_BINARY    = "./thread_library"
HTTP_PORT   = 8080
WS_PORT     = 8765
CLIENTS     = set()

# ── Broadcast ──
async def broadcast(message):
    if CLIENTS:
        data = json.dumps(message)
        disconnected = set()
        for ws in CLIENTS:
            try:
                await ws.send(data)
            except:
                disconnected.add(ws)
        CLIENTS.difference_update(disconnected)

# ── Classify line type ──
def classify(text):
    if "PASS"         in text: return "pass"
    if "FAIL"         in text: return "fail"
    if "ERROR"        in text: return "error"
    if "CANCELLED"    in text: return "fail"
    if "Cancellation" in text: return "fail"
    if "AutoScale"    in text: return "stats"
    if "Resize"       in text: return "stats"
    if "Created"      in text: return "create"
    if "Joined"       in text: return "join"
    if "Spawned"      in text: return "create"
    if "Locked"       in text: return "sync"
    if "Unlocked"     in text: return "sync"
    if "Pool"         in text: return "pool"
    if "Thread"       in text: return "thread"
    if "Worker"       in text: return "worker"
    if "Throughput"   in text or "tasks/sec" in text: return "perf"
    if "counter"      in text: return "perf"
    if "✓"            in text or "correctly" in text: return "success"
    if "═"            in text or "╔" in text or "╚" in text or "║" in text: return "box"
    if "MODULE"       in text: return "section"
    if "SCALABILITY"  in text: return "section"
    if "PRIORITY"     in text: return "section"
    if "COMPARISON"   in text: return "section"
    if "FEATURE"      in text: return "section"
    if "CLEANUP"      in text: return "section"
    if "BONUS"        in text: return "section"
    if "[+]"          in text: return "info"
    if "Stats"        in text: return "stats"
    if "Speedup"      in text: return "perf"
    return "output"

# ── Run C binary and stream ──
async def run_c_binary():
    await asyncio.sleep(0.3)
    await broadcast({"type": "status", "msg": "$ ./thread_library"})

    if not Path(C_BINARY).exists():
        await broadcast({"type": "error", "msg": f"Binary not found: {C_BINARY}. Run 'make' first!"})
        return

    proc = await asyncio.create_subprocess_exec(
        C_BINARY,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.STDOUT
    )

    async for line in proc.stdout:
        text = line.decode('utf-8', errors='replace').rstrip()
        if not text:
            await broadcast({"type": "blank"})
            continue
        await broadcast({"type": classify(text), "msg": text})
        await asyncio.sleep(0.005)

    await proc.wait()
    await broadcast({"type": "done", "msg": "── Program finished ──"})

# ── Run make test ──
async def run_make_test():
    await broadcast({"type": "status", "msg": "$ make test"})
    proc = await asyncio.create_subprocess_exec(
        "make", "test",
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.STDOUT
    )
    async for line in proc.stdout:
        text = line.decode('utf-8', errors='replace').rstrip()
        if not text:
            await broadcast({"type": "blank"})
            continue
        await broadcast({"type": classify(text), "msg": text})
        await asyncio.sleep(0.02)
    await proc.wait()
    await broadcast({"type": "done", "msg": "── Tests finished ──"})

# ── WebSocket handler ──
async def ws_handler(websocket):
    CLIENTS.add(websocket)
    print(f"[WS] Browser connected | total: {len(CLIENTS)}")
    try:
        await broadcast({"type": "status", "msg": "Connected to Thread Library server ✓"})
        async for message in websocket:
            data = json.loads(message)
            if data.get("cmd") == "run":
                asyncio.create_task(run_c_binary())
            elif data.get("cmd") == "test":
                asyncio.create_task(run_make_test())
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        CLIENTS.discard(websocket)
        print(f"[WS] Browser disconnected | total: {len(CLIENTS)}")

# ── HTTP server ──
class SilentHandler(SimpleHTTPRequestHandler):
    def log_message(self, format, *args): pass
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            self.path = '/gui/dashboard_connected.html'
        super().do_GET()

def start_http():
    os.chdir(Path(__file__).parent)
    server = HTTPServer(('', HTTP_PORT), SilentHandler)
    print(f"[HTTP] Serving on http://localhost:{HTTP_PORT}")
    server.serve_forever()

# ── Main ──
async def main():
    print("╔══════════════════════════════════════════╗")
    print("║   Thread Library — Bridge Server         ║")
    print("╚══════════════════════════════════════════╝")
    print()

    t = threading.Thread(target=start_http, daemon=True)
    t.start()

    print(f"[WS]   Listening on ws://localhost:{WS_PORT}")
    print()
    print("  Open your browser: http://localhost:8080")
    print("  Press Ctrl+C to stop")
    print()

    async with websockets.serve(ws_handler, "localhost", WS_PORT):
        await asyncio.Future()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[Server] Stopped.")
