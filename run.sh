#!/bin/sh
version="1.1008";

if [[ -z $(docker images -a | grep "trader_base_$version ") ]] ; then
    docker images -a | grep trader_ | awk '{print $1}' | xargs docker rmi
    docker build --tag "trader_base_$version" DockerBase;

    data=$(sed "1s/.*/FROM trader_base_$version/" ./Dockerfile)
    echo "${data}" > ./Dockerfile;

    docker rmi -f trader;
    docker build --tag trader .;
    docker rm trader;
fi

if [ "$1" = "test" ] ; then
    docker run -v `pwd`/:/trader -it -e "RUN_TESTS=1" --rm trader
else
    docker run -v `pwd`/:/trader -it -e "RUN_TESTS=0" --rm trader
fi
