#include <stdlib.h>
#include <string.h>
#include "base/Allocator.h"
#include "net/SocketPoolTaskRunner.h"

std::string coin_name;
int main(int argc, char* argv[])
{
    if (argc < 2 || strlen(argv[1]) < 1) {
        fprintf(stderr, "First parameter must be a valid coin abbr name.\n");
        exit(1);
    }

    coin_name = argv[1];

    net::SocketPoolTaskRunner::start();
}