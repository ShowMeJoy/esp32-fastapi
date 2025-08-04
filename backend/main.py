from fastapi import FastAPI, Query
from fastapi.responses import JSONResponse  # for work with status coded
from pydantic import BaseModel
from typing import Optional, List   # specific types
from sqlmodel import Field, SQLModel, create_engine, Session, select
from datetime import datetime, UTC

# uvicorn main:app --host 0.0.0.0 --port 8000


class SensorData(BaseModel):
    device_id: str = 'esp32-1'
    temperature: float
    humidity: float
    pressure: int
    altitude: float
    timestamp: int


class Measurement(SQLModel, table=True):
    id: Optional[int] = Field(default=None, primary_key=True)
    device_id: str
    temperature: float
    humidity: float
    pressure: int
    altitude: float
    timestamp: int


# SQLite file
engine = create_engine("sqlite:///database.db")


app = FastAPI()
data_log: list[SensorData] = []
log_message: list[str] = []


# create datasheet
@app.on_event("startup")
def on_startup():
    SQLModel.metadata.create_all(engine)


# POST: add update
@app.post("/submit/")
def submit_data(measurement: Measurement):
    if measurement.temperature > 40.0:
        return JSONResponse(
            status_code=400,
            content={"error": "Overheat detected!"}
        )
    with Session(engine) as session:
        session.add(measurement)
        session.commit()
    return JSONResponse(
        status_code=201,
        content={"status": "saved"}
    )


# GET /log/ - all data
# GET /log/?device_id=esp32-1 - only from one device
# GET /log/?since=1753700000 - since timestamp
# GET /log/?device_id=esp32-1&since=1753700000 - both of filters
@app.get("/log/", response_model=List[Measurement])
def get_log(device_id: Optional[str] = Query(None), since: Optional[int] = Query(None)):
    with Session(engine) as session:
        stmt = select(Measurement)
        if device_id:
            stmt = stmt.where(Measurement.device_id == device_id)
        if since:
            stmt = stmt.where(Measurement.timestamp >= since)
        result = session.exec(stmt).all()
        return result


# log for reading with comfortable date format
@app.get("/log/human/")
def get_log_human_readable():
    with Session(engine) as session:
        rows = session.exec(select(Measurement)).all()
        return [
            {
            "device_id": r.device_id,
            "temperature": r.temperature,
            "humidity": r.humidity,
            "timestamp": r.timestamp,
            "time": datetime.fromtimestamp(r.timestamp, UTC)
            }
            for r in rows
        ]

