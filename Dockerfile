FROM gcc

RUN mkdir /src
WORKDIR /src

RUN mkdir /config
ADD ./config /config/

ADD ./src /src/

RUN mkdir /var/log/erss
RUN echo ERSS Log >> /var/log/erss/proxy.log

RUN apt-get update

RUN chmod +x /config/entrypoint.sh
RUN chmod +x server.cpp
RUN chmod a+rw /var/log/erss/proxy.log

RUN g++ -std=c++11 -g -o server server.cpp
