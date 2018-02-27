FROM gcc

RUN mkdir /src
WORKDIR /src

RUN mkdir /config
ADD ./config /config/

ADD ./src /src/

RUN mkdir /var/log/erss
RUN echo ERSS Log >> /var/log/erss/proxy.log

RUN apt-get update && apt-get install -y libpoco-dev

RUN chmod +x /config/entrypoint.sh
RUN chmod +x proxy_server.cpp
RUN chmod a+rw /var/log/erss/proxy.log

RUN rm proxy_server
RUN g++ -o proxy_server proxy_server.cpp -lPocoNet -lPocoUtil -lPocoFoundation -std=c++11
