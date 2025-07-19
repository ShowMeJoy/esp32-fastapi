from fastapi import FastAPI
from fastapi.responses import JSONResponse
from pydantic import BaseModel

# uvicorn main:app --host 0.0.0.0 --port 8000


class SensorData(BaseModel):
    device_id: str = 'esp32-1'
    temperature: float
    humidity: float
    timestamp: str


app = FastAPI()
data_log: list[SensorData] = []
log_message: list[str] = []


@app.post("/log/")
def receive_data(data: SensorData):
    log_entry = f"Device={data.device_id} | Temp={data.temperature} C | Hum={data.humidity} | Time={data.timestamp}"
    log_message.append(log_entry)
    if data.temperature > 40:
        return JSONResponse(
            status_code=400,
            content={"error": "Overheat detected"}
        )
    data_log.append(data)
    print(f"{data.timestamp}: {data.temperature} C, {data.humidity} %")
    return JSONResponse(
        status_code=201,
        content={"status": "ok"}
    )


@app.get("/log/")
def get_log():
    return data_log


@app.get("/log/message")
def get_message():
    return log_message