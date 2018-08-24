CC = g++
CFLAGS = -std=c++11 -lpthread
SOURCES_SERVER = db/server_main.cpp db/data_base.cpp db/storage_buffer.cpp db/storage_engine.cpp structure/cache.cpp structure/memory.cpp util/coding.cpp util/endian.cpp util/log_level.cpp type/order_type.cpp type/constant.cpp
SOURCES_CLIENT = benchmark/client_main.cpp util/coding.cpp util/endian.cpp util/sequence_generator.cpp type/order_type.cpp
SOURCES_DB_BENCHMARK = benchmark/db_benchmark_main.cpp db/data_base.cpp db/storage_buffer.cpp db/storage_engine.cpp structure/cache.cpp structure/memory.cpp util/coding.cpp util/sequence_generator.cpp util/endian.cpp util/log_level.cpp type/order_type.cpp type/constant.cpp

all : client_main server_main db_benchmark_main
.PHONY : all

client_main : $(SOURCES_CLIENT) $(OBJECTS)
	$(CC) $(CFLAGS) $(SOURCES_CLIENT) $(OBJECTS) -o $@ 

server_main : $(SOURCES_SERVER)
	$(CC) $(CFLAGS) $(SOURCES_SERVER) -o $@

db_benchmark_main : $(SOURCES_DB_BENCHMARK)
	$(CC) $(CFLAGS) $(SOURCES_DB_BENCHMARK) -o $@

.PHONY : clean
clean : 
	rm client_main server_main db_benchmark_main
