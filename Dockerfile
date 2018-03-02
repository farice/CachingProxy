FROM gcc

RUN mkdir /src
WORKDIR /src

RUN mkdir /config
ADD ./config /config/

RUN mkdir /var/log/erss
ADD ./logs /var/log/erss

RUN apt-get update && apt-get install -y libpoco-dev

ADD ./src /src

RUN chmod +x /config/entrypoint.sh
RUN chmod a+rx proxy_server.cpp
RUN chmod a+rw /var/log/erss
