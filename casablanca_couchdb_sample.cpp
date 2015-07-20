// Producer/consumer bulk upload to CouchDB using Microsoft Casablanca REST library and Boost

// compiled with locally built casablanca:
// https://casablanca.codeplex.com
// clang++ casablanca_couchdb_sample.cpp -std=c++11 -I../casablanca/Release/include/ -lcpprest -L../casablanca//build.release/Binaries/ -lboost_thread-mt -lboost_system -lboost_chrono -lboost_program_options

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/format.hpp>
#include <boost/atomic.hpp>
#include <boost/foreach.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>

#include <iostream>


using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams


int iterations = 100000;

int batch_size = 10;

boost::lockfree::spsc_queue<web::json::value> queue(128);

boost::atomic_int producer_count(0);
boost::atomic_int consumer_count(0);

boost::atomic<bool> done (false);

void send_batch(std::vector<web::json::value> &batch) {

    web::json::value v = web::json::value::object();
    v["docs"] = web::json::value::array();

    int i=0;
    BOOST_FOREACH(web::json::value batch_object, batch) {
        v["docs"][i++] = batch_object;
    }

    // Create http_client to send the request.
    http_client client(U("http://127.0.0.1:5984/testdb"));
    
    // Build request URI and start the request.
    uri_builder builder(U("_bulk_docs"));

    pplx::task<void> requestTask = client.request(methods::POST, builder.to_string(), v)
    
    // Handle response headers arriving.
    .then([=](http_response response)
    {
#ifdef DEBUG
        printf("Received response status code:%u\n", response.status_code());
        std::cout << response.to_string();
#endif
        return;
    });

    // Wait for all the outstanding I/O to complete and handle any exceptions
    try
    {
        requestTask.wait();
    }
    catch (const std::exception &e)
    {
        printf("Error exception:%s\n", e.what());
    }

}

void producer(void)
{
    for (int i = 0; i != iterations; ++i) {

        web::json::value doc = web::json::value::object();
        doc["_id"] = web::json::value::string(str(boost::format("FishStew %d") % i));
        doc["servings"] = web::json::value::number(4);
        doc["subtitle"] = web::json::value::string("Delicious with freshly baked bread");
        doc["title"] = web::json::value::string("FishStew");

        while (!queue.push(doc))
        ; // queue full, busy wait

        ++producer_count;
    }
}

void consumer(void)
{
    web::json::value value;
    int batch = 0;
    std::vector<web::json::value> batch_to_send;
    while (!done) {
        while (queue.pop(value)) {
            ++consumer_count;
            batch_to_send.push_back(value);
            if (++batch == batch_size) {
                batch = 0;
                send_batch(batch_to_send);
                batch_to_send.clear();
            }
        }
    }
}

int main(int argc, char* argv[])
{

    // Declare the supported options.
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce this help message")
        ("bs", boost::program_options::value<int>(), "batch size")
        ("d", boost::program_options::value<int>(), "number of documents")
        ;
    
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);    
    
    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }
    if (vm.count("bs")) {
        batch_size = vm["bs"].as<int>();
    }
    if (vm.count("d")) {
        iterations = vm["d"].as<int>();
    }

    
    boost::chrono::steady_clock::time_point start=boost::chrono::steady_clock::now();
   
    boost::thread producer_thread(producer);
    boost::thread consumer_thread(consumer);
                                      
    producer_thread.join();
    done = true;

    consumer_thread.join();

    std::cout << "produced " << producer_count << " objects." << std::endl;
    std::cout << "consumed " << consumer_count << " objects." << std::endl;

    std::cout << "batch size was " << batch_size << " objects." << std::endl;
    
    boost::chrono::duration<double> d = boost::chrono::steady_clock::now() - start;
    // d now holds the number of milliseconds from start to end.
    std::cout << "took " << d.count() << " seconds\n" << std::endl;
    std::cout << "rate " << (consumer_count / d.count()) << " docs / second" << std::endl;
    
    return 0;
}
