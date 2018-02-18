FROM gcc

RUN mkdir /src
WORKDIR /src

RUN mkdir /config
ADD ./config /config/

ADD ./src /src/

RUN apt-get update

RUN chmod +x /config/entrypoint.sh
RUN chmod +x server.cpp
RUN g++ -std=c++11 -g -o server server.cpp
