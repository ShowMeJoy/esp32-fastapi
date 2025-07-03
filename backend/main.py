from fastapi import FastAPI
from pydantic import BaseModel

app = FastAPI()


class SensorData(BaseModel):
    temperature: float
    humidity: float
    timestamp: str


@app.post("/log")
def receive_data(data: SensorData):
    print(f"{data.timestamp}: {data.temperature} C, {data.humidity} %")
    return {"status": "ok"}