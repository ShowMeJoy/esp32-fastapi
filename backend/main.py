from fastapi import FastAPI
from fastapi.responses import JSONResponse  # for work with status coded
from pydantic import BaseModel
from typing import Optional, List   # specific types
from sqlmodel import Field, SQLModel, create_engine, Session, select

# uvicorn main:app --host 0.0.0.0 --port 8000


class SensorData(BaseModel):
    device_id: str = 'esp32-1'
    temperature: float
    humidity: float
    timestamp: int


class Measurement(SQLModel, table=True):
    id: Optional[int] = Field(default=None, primary_key=True)
    device_id: str
    temperature: float
    humidity: float
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
    with Session(engine) as session:
        session.add(measurement)
        session.commit()
    return {"status": "saved"}


# GET: return all
@app.get("/log/", response_model=List[Measurement])
def get_log():
    with Session(engine) as session:
        result = session.exec(select(Measurement)).all()
        return result