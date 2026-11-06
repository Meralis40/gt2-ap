#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""A stand-in for an Archipelago room, to test the client without a generated seed.

Only the standard library. It speaks plain ws, gives the slot every item listed in --items
in one ReceivedItems and prints what the client reports. It is no substitute for a real
server: it knows nothing of the rules, and only answers the packets the client sends.

  fake_ap_server.py [--port 38281] [--items 1,1,104,300] [--password x] [--refuse-slot x]
                    [--drop-after <seconds>] [--checked 1137,1138]
"""
import argparse
import asyncio
import base64
import hashlib
import json
import struct
import time

GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


def frame(text):
    data = text.encode()
    if len(data) < 126:
        head = bytes([0x81, len(data)])
    elif len(data) < 65536:
        head = bytes([0x81, 126]) + struct.pack(">H", len(data))
    else:
        head = bytes([0x81, 127]) + struct.pack(">Q", len(data))
    return head + data


async def read_frame(reader):
    first, second = await reader.readexactly(2)
    size = second & 0x7f
    if size == 126:
        size = struct.unpack(">H", await reader.readexactly(2))[0]
    elif size == 127:
        size = struct.unpack(">Q", await reader.readexactly(8))[0]
    if not second & 0x80:
        raise ValueError("client frames must be masked")
    mask = await reader.readexactly(4)
    payload = await reader.readexactly(size)
    return first & 0xf, bytes(b ^ mask[i & 3] for i, b in enumerate(payload))


def log(*args):
    print(f"[{time.strftime('%H:%M:%S')}]", *args, flush=True)


class Room:
    def __init__(self, args):
        self.args = args
        self.items = [int(i) for i in args.items.split(",") if i]
        self.checked = {int(i) for i in args.checked.split(",") if i}

    def connected(self):
        return {"cmd": "Connected", "team": 0, "slot": 1,
                "players": [{"team": 0, "slot": 1, "alias": "Tester", "name": "Tester"}],
                "missing_locations": [], "checked_locations": sorted(self.checked),
                "slot_info": {"1": {"name": "Tester", "game": "Gran Turismo 2", "type": 1, "group_members": []}},
                "hint_points": 0,
                "slot_data": {"options": {"league_trophies": 1, "pro_league_trophies": 2, "world_league_trophies": 3},
                              "start_manufacturer": "Mazda", "start_manufacturer_id": 20}}

    def answer(self, packet):
        cmd = packet["cmd"]
        if cmd == "GetDataPackage":
            log("data package for", packet.get("games"))
            names = {"Progressive License": 1, "Trophy": 2}
            return [{"cmd": "DataPackage", "data": {"games": {"Gran Turismo 2": {
                "item_name_to_id": names, "location_name_to_id": {}, "checksum": "0"}}}}]
        if cmd == "Connect":
            log("connect", packet["name"], "password", "yes" if packet.get("password") else "no",
                "items_handling", packet["items_handling"])
            if self.args.password and packet.get("password") != self.args.password:
                return [{"cmd": "ConnectionRefused", "errors": ["InvalidPassword"]}]
            if packet["name"] == self.args.refuse_slot:
                return [{"cmd": "ConnectionRefused", "errors": ["InvalidSlot"]}]
            return [self.connected(),
                    {"cmd": "ReceivedItems", "index": 0,
                     "items": [{"item": i, "location": 0, "player": 1, "flags": 0} for i in self.items]},
                    {"cmd": "PrintJSON", "type": "ItemSend", "receiving": 1,
                     "item": {"item": 1, "location": 0, "player": 1, "flags": 1},
                     "data": [{"type": "player_id", "text": "1"}, {"type": "text", "text": " found their "},
                              {"type": "item_id", "text": "1", "player": 1, "flags": 1}]}]
        if cmd == "LocationChecks":
            new = sorted(set(packet["locations"]) - self.checked)
            self.checked.update(packet["locations"])
            log("locations", packet["locations"], "new", new)
            return [{"cmd": "RoomUpdate", "checked_locations": new}] if new else []
        if cmd == "StatusUpdate":
            log("status", packet["status"])
            return []
        if cmd == "Sync":
            log("sync")
            return []
        log("unhandled", cmd)
        return []

    async def client(self, reader, writer):
        request = await reader.readuntil(b"\r\n\r\n")
        lines = request.decode().split("\r\n")
        headers = {k.lower(): v for k, _, v in (line.partition(": ") for line in lines[1:])}
        accept = base64.b64encode(hashlib.sha1((headers["sec-websocket-key"] + GUID).encode()).digest()).decode()
        writer.write(("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                      f"Sec-WebSocket-Accept: {accept}\r\n\r\n").encode())
        writer.write(frame(json.dumps([{"cmd": "RoomInfo", "seed_name": "FAKE1234", "password": False,
                                        "games": ["Gran Turismo 2"], "datapackage_checksums": {},
                                        "version": {"major": 0, "minor": 6, "build": 8, "class": "Version"}}])))
        await writer.drain()
        log("client connected")
        deadline = time.monotonic() + self.args.drop_after if self.args.drop_after else None
        try:
            while True:
                timeout = None if deadline is None else max(0.01, deadline - time.monotonic())
                opcode, payload = await asyncio.wait_for(read_frame(reader), timeout)
                if opcode == 8:
                    break
                if opcode != 1:
                    continue
                replies = []
                for packet in json.loads(payload):
                    replies += self.answer(packet)
                if replies:
                    writer.write(frame(json.dumps(replies)))
                    await writer.drain()
        except (asyncio.IncompleteReadError, ConnectionError):
            pass
        except asyncio.TimeoutError:
            log("dropping the client, as asked")
            self.args.drop_after = 0
        writer.close()
        log("client gone")


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=38281)
    parser.add_argument("--items", default="")
    parser.add_argument("--password", default="")
    parser.add_argument("--refuse-slot", default="")
    parser.add_argument("--drop-after", type=float, default=0)
    parser.add_argument("--stop-after", type=float, default=0)
    parser.add_argument("--checked", default="")
    args = parser.parse_args()
    server = await asyncio.start_server(Room(args).client, "127.0.0.1", args.port)
    log("listening on", args.port)
    async with server:
        if args.stop_after:
            await asyncio.sleep(args.stop_after)
        else:
            await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(main())
