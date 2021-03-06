#!/bin/sh
version="1.1010";

if [[ -z $(docker images -a | grep "trader_base_$version ") ]] ; then
    docker images -a | grep trader_ | awk '{print $1}' | xargs docker rmi
    docker build --tag "trader_base_$version" DockerBase;

    data=$(sed "1s/.*/FROM trader_base_$version/" ./Dockerfile)
    echo "${data}" > ./Dockerfile;

    docker rmi -f trader;
    docker build --tag trader .;
    docker rm trader;
fi

ENV_PARAMS=""

for ARG in "$@"
do
    case "$ARG" in 
        "debug")
            ENV_PARAMS="$ENV_PARAMS -e 'DEBUG_OUTPUT=1'";;
        "test")
            ENV_PARAMS="$ENV_PARAMS -e 'RUN_TESTS=1'";;
        "optimized")
            ENV_PARAMS="$ENV_PARAMS -e 'OPRIMIZED=1'";;
        "bash")
            ENV_PARAMS="$ENV_PARAMS -e 'RUN_BASH=1'";;
        "valgrind")
            ENV_PARAMS="$ENV_PARAMS -e 'VALGRIND=1'";;
    esac
done

eval "docker run -v `pwd`/:/trader -it ${ENV_PARAMS} --rm trader"
