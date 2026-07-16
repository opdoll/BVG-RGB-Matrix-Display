from fastapi import FastAPI
from fastapi.responses import PlainTextResponse

app = FastAPI()
message = "Hello BVG!"

@app.get("/message", response_class=PlainTextResponse)
async def get_message():
    return message
