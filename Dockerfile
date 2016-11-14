FROM ubuntu

ADD . /trader/

#RUN apt-get update && apt-get -y install clang-3.8 cmake build-essential

RUN apt-get update && apt-get -y install cmake build-essential

#ENV CC /usr/bin/clang-3.8

RUN cd /trader; cmake .

# docker build --tag trader .; docker rm trader; docker create --name trader trader; docker run -i -t trader bash