#include "producer.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <rdkafka.h>

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <stdexcept>

using json = nlohmann::json;

// ---------------- HTTP utils ----------------
static size_t write_cb(void* contents, size_t size, size_t nmemb, void* userp)
{
    auto* s = static_cast<std::string*>(userp);
    s->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

static std::string httpGet(const std::string& url)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("curl_easy_init failed");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string err = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error("curl_easy_perform failed: " + err);
    }

    curl_easy_cleanup(curl);
    return response;
}

// ---------------- Producer (Kafka) ----------------
Producer::Producer(const std::string& brokers,
    const std::string& topic,
    const std::string& twelveApiKey)
    : brokers_(brokers)
    , topicName_(topic)
    , twelveApiKey_(twelveApiKey)
{
    char errstr[512];

    conf_ = rd_kafka_conf_new();
    if (!conf_)
        throw std::runtime_error("rd_kafka_conf_new failed");

    if (rd_kafka_conf_set(conf_, "bootstrap.servers",
        brokers_.c_str(), errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
    {
        throw std::runtime_error(errstr);
    }

    rk_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf_, errstr, sizeof(errstr));
    if (!rk_)
        throw std::runtime_error(errstr);

    conf_ = nullptr;

    topic_ = rd_kafka_topic_new(rk_, topicName_.c_str(), nullptr);
    if (!topic_)
        throw std::runtime_error("rd_kafka_topic_new failed");

    std::cout << "[Producer] Kafka ready on " << brokers_
        << " topic=" << topicName_ << std::endl;
}

Producer::~Producer()
{
    stop();

    if (rk_) {
        rd_kafka_flush(rk_, 3000);
        rd_kafka_topic_destroy(topic_);
        rd_kafka_destroy(rk_);
    }
}

// ---------------- Kafka send ----------------
void Producer::sendRaw(const std::string& msg)
{
    if (msg.empty())
        return;

    if (rd_kafka_produce(
        topic_,
        RD_KAFKA_PARTITION_UA,
        RD_KAFKA_MSG_F_COPY,
        (void*)msg.data(),
        msg.size(),
        nullptr,
        0,
        nullptr) != 0)
    {
        std::cerr << "[Kafka] produce failed: "
            << rd_kafka_err2str(rd_kafka_last_error()) << std::endl;
    }

    rd_kafka_poll(rk_, 0);
}

void Producer::sendTick(const MarketDataTick& t)
{
    std::ostringstream ss;
    ss << t.symbol << ";"
        << t.last_price << ";"
        << t.open << ";"
        << t.high << ";"
        << t.low << ";"
        << t.volume << ";"
        << t.timestamp;

    sendRaw(ss.str());
}

// ---------------- Twelve Data ----------------
void Producer::startTwelveData(const std::string& symbol, int intervalSec)
{
    if (running_.load())
        return;

    running_.store(true);
    worker_ = std::thread(&Producer::twelveDataLoop, this, symbol, intervalSec);
}

void Producer::stop()
{
    running_.store(false);
    if (worker_.joinable())
        worker_.join();
}

void Producer::twelveDataLoop(const std::string& symbol, int intervalSec)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);

    while (running_.load()) {
        try {
            std::ostringstream url;
            url << "https://api.twelvedata.com/time_series"
                << "?symbol=" << symbol
                << "&interval=1min"
                << "&apikey=" << twelveApiKey_;

            const json j = json::parse(httpGet(url.str()));

            if (j.contains("status") && j["status"] == "error") {
                std::cerr << "[TwelveData] "
                    << j.value("message", "error") << std::endl;
            }
            else if (j.contains("values") && !j["values"].empty()) {
                const auto& v = j["values"][0];

                MarketDataTick t;
                t.symbol = symbol;
                t.open = std::stod(v["open"].get<std::string>());
                t.high = std::stod(v["high"].get<std::string>());
                t.low = std::stod(v["low"].get<std::string>());
                t.last_price = std::stod(v["close"].get<std::string>());
                t.volume = std::stoll(v.value("volume", "0"));
                t.timestamp = std::time(nullptr);

                sendTick(t);

                std::cout << "[TwelveData] "
                    << t.symbol << " " << t.last_price << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[TwelveData] exception: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(intervalSec));
    }

    curl_global_cleanup();
}

