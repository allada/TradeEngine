#include <stdlib.h>
#include <string.h>
#include "base/Allocator.h"
#include "net/SocketPoolTaskRunner.h"

std::string coin_name;

void terminate()
{
    fprintf(stderr, "Terminating\n");
    net::SocketPoolTaskRunner::terminate();
    exit(1);
}

int main(int argc, char* argv[])
{
    // Must be attached first so the threads may be closed via chanel.
    net::SocketPoolTaskRunner::start();
    if (argc < 2 || strlen(argv[1]) < 1) {
        fprintf(stderr, "First parameter must be a valid coin abbr name.\n");
        terminate();
    }
    // TODO do something better.
    while(true) {};
    //coin_name = argv[1];
}