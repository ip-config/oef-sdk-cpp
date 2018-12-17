FROM ubuntu:18.04

USER root

RUN apt-get update && apt-get upgrade -y

RUN apt-get install -y clang && \
    apt-get install -y gcc && \
    apt-get install -y g++

RUN apt-get install -y make
RUN apt-get install -y cmake

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

RUN apt-get install protobuf-compiler -y &&\
    apt-get install libprotobuf-dev -y


WORKDIR /build
COPY . /build

RUN rm -Rf build &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make -j "$@" &&\
    make test

RUN cp -r ./build/apps /apps

WORKDIR /apps
