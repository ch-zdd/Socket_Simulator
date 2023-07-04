all:service client
	mv service client ./bin
service: ./src/service.c
	gcc ./src/service.c ./src/common.c ./src/common.h -o service -lpthread
client: ./src/client.c 
	gcc ./src/client.c ./src/common.c ./src/common.h -o client -lpthread
clean:
	rm -rf ./bin/*
