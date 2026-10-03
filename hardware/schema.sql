CREATE TABLE IF NOT EXISTS sensor_data (
    id BIGSERIAL PRIMARY KEY,
    device_id TEXT NOT NULL,
    timestamp TIMESTAMPTZ NOT NULL,
    temperature DOUBLE PRECISION NOT NULL,
    humidity DOUBLE PRECISION NOT NULL,
    pressure DOUBLE PRECISION NOT NULL,
    simulated BOOLEAN NOT NULL DEFAULT FALSE,
    UNIQUE (device_id, timestamp)
);
