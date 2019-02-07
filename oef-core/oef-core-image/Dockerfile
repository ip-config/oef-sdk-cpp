FROM ubuntu:18.04

USER root

RUN apt-get update && apt-get upgrade -y

RUN apt-get install -y clang && \
    apt-get install -y gcc && \
    apt-get install -y g++

RUN apt-get install -y make
RUN apt-get install -y cmake

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++

RUN apt-get install protobuf-compiler -y &&\
    apt-get install libprotobuf-dev -y

RUN apt-get install -y lcov


WORKDIR /source
COPY . /source

RUN rm -Rf build &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make -j4

RUN cp -r ./build/apps /apps

EXPOSE 3333
CMD ["/apps/node/OEFNode"]
