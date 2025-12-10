import time
import json
import requests
import threading
from confluent_kafka import Producer

API_KEY = "XQMM86M54ADXA2ZY"
KAFKA_BROKER = "localhost:9092"
TOPIC = "market_data"

producer = Producer({"bootstrap.servers": KAFKA_BROKER})


# -------------------------------------------------------------------------
# 1) UTILITAIRE : Conversion en payload C++ (format de ton MarketDataTick)
# -------------------------------------------------------------------------
def to_cpp_format(d):
    return (
        f"{d['symbol']};"
        f"{d['last_price']};"
        f"{d['open']};"
        f"{d['high']};"
        f"{d['low']};"
        f"{d['volume']};"
        f"{d['timestamp']}"
    )


# -------------------------------------------------------------------------
# 2) FETCH ACTIONS
# -------------------------------------------------------------------------
def fetch_stock(symbol):
    try:
        url = (f"https://www.alphavantage.co/query"
               f"?function=GLOBAL_QUOTE&symbol={symbol}&apikey={API_KEY}")

        r = requests.get(url).json()
        q = r.get("Global Quote")

        if not q:
            print(f"[ERROR STOCK {symbol}] empty data")
            return None

        return {
            "symbol": q.get("01. symbol", symbol),
            "timestamp": int(time.time()),
            "last_price": float(q.get("05. price", 0) or 0),
            "open": float(q.get("02. open", 0) or 0),
            "high": float(q.get("03. high", 0) or 0),
            "low": float(q.get("04. low", 0) or 0),
            "volume": int(q.get("06. volume", 0) or 0),
        }

    except Exception as e:
        print(f"[ERROR STOCK {symbol}] {e}")
        return None


# -------------------------------------------------------------------------
# 3) FOREX
# -------------------------------------------------------------------------
def fetch_fx(pair):
    try:
        base, quote = pair.split("/")
        url = (f"https://www.alphavantage.co/query"
               f"?function=CURRENCY_EXCHANGE_RATE"
               f"&from_currency={base}&to_currency={quote}&apikey={API_KEY}")

        r = requests.get(url).json()
        fx = r.get("Realtime Currency Exchange Rate")

        if not fx:
            print(f"[ERROR FX {pair}] empty data")
            return None

        price = float(fx.get("5. Exchange Rate", 0) or 0)

        return {
            "symbol": pair.replace("/", "_"),
            "timestamp": int(time.time()),
            "last_price": price,
            "open": price,
            "high": price,
            "low": price,
            "volume": 0
        }

    except Exception as e:
        print(f"[ERROR FX {pair}] {e}")
        return None


# -------------------------------------------------------------------------
# 4) CRYPTO
# -------------------------------------------------------------------------
def fetch_crypto(symbol):
    try:
        url = (f"https://www.alphavantage.co/query"
               f"?function=DIGITAL_CURRENCY_DAILY"
               f"&symbol={symbol}&market=USD&apikey={API_KEY}")

        r = requests.get(url).json()
        data = r.get("Time Series (Digital Currency Daily)")

        if not data:
            print(f"[ERROR CRYPTO {symbol}] empty data")
            return None

        latest_day = list(data.keys())[0]
        d = data[latest_day]

        return {
            "symbol": symbol,
            "timestamp": int(time.time()),
            "last_price": float(d.get("4b. close (USD)", 0) or 0),
            "open": float(d.get("1b. open (USD)", 0) or 0),
            "high": float(d.get("2b. high (USD)", 0) or 0),
            "low": float(d.get("3b. low (USD)", 0) or 0),
            "volume": int(d.get("5. volume", 0) or 0)
        }

    except Exception as e:
        print(f"[ERROR CRYPTO {symbol}] {e}")
        return None


# -------------------------------------------------------------------------
# 5) THREAD GENÉRIQUE
# -------------------------------------------------------------------------
def run_feed(name, fetch_fn, symbol, interval):
    while True:
        data = fetch_fn(symbol)
        if data:
            msg = to_cpp_format(data)
            print(f"[KAFKA] {msg}")
            producer.produce(TOPIC, msg)
        time.sleep(interval)


# -------------------------------------------------------------------------
# 6) MAIN MULTI-THREAD FEED
# -------------------------------------------------------------------------
if __name__ == "__main__":
    print("=== Alpha Vantage Multi-Feed → Kafka ===")

    threads = [

        # Stocks
        threading.Thread(target=run_feed, args=("AAPL", fetch_stock, "AAPL", 20)),
        threading.Thread(target=run_feed, args=("TSLA", fetch_stock, "TSLA", 20)),
        threading.Thread(target=run_feed, args=("MSFT", fetch_stock, "MSFT", 20)),

        # Forex
        threading.Thread(target=run_feed, args=("EUR/USD", fetch_fx, "EUR/USD", 30)),
        threading.Thread(target=run_feed, args=("GBP/USD", fetch_fx, "GBP/USD", 30)),

        # Crypto
        threading.Thread(target=run_feed, args=("BTC", fetch_crypto, "BTC", 60)),
        threading.Thread(target=run_feed, args=("ETH", fetch_crypto, "ETH", 60)),
    ]

    for t in threads:
        t.daemon = True
        t.start()

    while True:
        time.sleep(1)
