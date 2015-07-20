# C++ Casablanca CouchDB Sample

_Producer/consumer bulk upload to CouchDB using Microsoft Casablanca REST library and Boost_

# Build and run

_Pre-requisites_

- a C++11 compiler
- boost[http://www.boost.org]
- Casablanca[https://casablanca.codeplex.com]
- a copy of CouchDB[http://couchdb.apache.org] running somewhere

_Building_

This will depend on platform, but on the Mac it looks something like
```
clang++ casablanca_couchdb_sample.cpp -std=c++11 -I$CASABLANCA_DIR/Release/include/ -lcpprest -L$CASABLANCA_DIR/build.release/Binaries/ -lboost_thread-mt -lboost_system -lboost_chrono -lboost_program_options
```

_The sample shows:_

- Creating JSON documents using Casablanca
- Making REST API calls using Casablanca
- Using boost threading primitives to create and upload batches of documents using a producer/consumer pattern and a shared queue

Sample showing an upload of 100000 documents with a batch size of 1000:

```shell
14:30:45 in cplusplus$ curl -XDELETE "http://127.0.0.1:5984/testdb" && curl -XPUT "http://127.0.0.1:5984/testdb" && ./a.out --d 100000 --bs 1000
{"ok":true}
{"ok":true}
produced 100000 objects.
consumed 100000 objects.
batch size was 1000 objects.
took 10.9801 seconds

rate 9107.36docs / second
```

# Issues

- If the number of documents is not exactly divisible by the batch size, the remainder do not get uploaded (!)
- Database URL is hard-coded
- Database is not deleted/created as part of the test run
