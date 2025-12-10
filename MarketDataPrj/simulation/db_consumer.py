# -*- coding: utf-8 -*-

from confluent_kafka import Consumer
import psycopg2
import json

print("Starting DB consumer...")

# 1. PostgreSQL connection
conn = psycopg2.connect(
    dbname="market_data_db",
    user="postgres",
    password="admin123",  # change if needed
    host="localhost",
    port="5433"
)
cursor = conn.cursor()

# 2. Kafka Consumer configuration
consumer = Consumer({
    'bootstrap.servers': 'localhost:9092',
    'group.id': 'db-writer',
    'auto.offset.reset': 'earliest'
})

consumer.subscribe(['market_data'])

print("Connected to Kafka topic market_data.")

while True:
    msg = consumer.poll(1.0)

    if msg is None:
        continue

    if msg.error():
        print("Kafka Error:", msg.error())
        continue

    data = json.loads(msg.value().decode("utf-8"))

    cursor.execute("""
        INSERT INTO ticks(symbol, ts, last_price, volume, open, high, low)
        VALUES (%s, to_timestamp(%s), %s, %s, %s, %s, %s)
    """, (
        data["symbol"],
        data["timestamp"],
        data.get("last_price"),
        data.get("volume"),
        data.get("open"),
        data.get("high"),
        data.get("low")
    ))

    conn.commit()
    print("Inserted:", data)
