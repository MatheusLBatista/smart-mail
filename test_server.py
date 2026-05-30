#!/usr/bin/env python3
"""
Servidor de teste para o frontend do Sensor de Correio.

Instale a dependência:
    pip install aiohttp

Execute:
    python3 test_server.py

Acesse:
    http://localhost:8080

Para disparar um evento manualmente (em outro terminal):
    curl -X POST http://localhost:8080/trigger
"""

import asyncio
import json
import datetime
import pathlib
from aiohttp import web

HTML_PATH = pathlib.Path(__file__).parent / "data" / "index.html"
clientes: set[web.WebSocketResponse] = set()


async def enviar_evento():
    agora = datetime.datetime.now()
    msg = json.dumps({
        "type": "mail",
        "time": agora.strftime("%H:%M:%S"),
        "date": agora.strftime("%d/%m/%Y"),
    })
    mortos = set()
    for ws in list(clientes):
        try:
            await ws.send_str(msg)
        except Exception:
            mortos.add(ws)
    clientes.difference_update(mortos)
    print(f"[FAKE] Evento disparado às {agora.strftime('%H:%M:%S')} "
          f"para {len(clientes)} cliente(s)")


async def handler_index(request):
    return web.Response(
        text=HTML_PATH.read_text(encoding="utf-8"),
        content_type="text/html"
    )


async def handler_ws(request):
    ws = web.WebSocketResponse()
    await ws.prepare(request)
    clientes.add(ws)
    print(f"[WS] Cliente conectado  — total: {len(clientes)}")
    async for _ in ws:
        pass
    clientes.discard(ws)
    print(f"[WS] Cliente desconectado — total: {len(clientes)}")
    return ws


async def handler_trigger(request):
    """POST /trigger — simula a chegada de um correio."""
    await enviar_evento()
    return web.Response(text="ok")


async def loop_automatico():
    """Dispara um evento fake a cada 30 s se houver clientes conectados."""
    while True:
        await asyncio.sleep(30)
        if clientes:
            print("[AUTO] Disparando evento automático...")
            await enviar_evento()


async def main():
    app = web.Application()
    app.router.add_get("/",        handler_index)
    app.router.add_get("/ws",      handler_ws)
    app.router.add_post("/trigger", handler_trigger)

    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, "0.0.0.0", 8080)
    await site.start()

    print("=" * 50)
    print("  Servidor de teste rodando!")
    print("  Frontend:  http://localhost:8080")
    print("  Trigger:   curl -X POST http://localhost:8080/trigger")
    print("  Auto:      evento a cada 30s se tiver alguém conectado")
    print("  Parar:     Ctrl+C")
    print("=" * 50)

    await loop_automatico()


if __name__ == "__main__":
    asyncio.run(main())
