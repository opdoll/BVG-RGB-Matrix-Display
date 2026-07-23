from datetime import datetime
from math import ceil

import httpx
from fastapi import FastAPI, HTTPException

app = FastAPI()

VBB_API_URL = "https://v6.vbb.transport.rest"

config = {
    "stopId": "900086107", #uranusweg
    "results": 3,
}

@app.get("/message")
async def get_message():
    departures_url = VBB_API_URL + f"/stops/{config['stopId']}/departures"
    params = {
        "results": config["results"],
    }

    try:
        async with httpx.AsyncClient(timeout=10) as client:
            response = await client.get(departures_url, params=params)

        response.raise_for_status()
        departures = response.json()["departures"]

        result = []
        for departure in departures[:3]:
            departure_time = datetime.fromisoformat(departure["when"] or departure["plannedWhen"])
            minutes = max(0, ceil((departure_time - datetime.now(departure_time.tzinfo)).total_seconds() / 60))
            result.append({
                "line": departure["line"]["name"],
                "direction": departure["direction"],
                "mins": minutes,
            })

        return {"departures": result}

    except (httpx.HTTPError, KeyError, TypeError, ValueError) as exc:
        raise HTTPException(status_code=502, detail=f"External API failed: {exc}")
