#!/usr/bin/env bash
echo Proxy server is up

echo Testing basic HTTP...
curl -v -x http://127.0.0.1:12345 http://www.example.com/
echo Proxy log:
tail ../logs/proxy.log

echo Testing concurrent HTTP...
curl -v -x http://127.0.0.1:12345 http://www.example.com/
curl -v -x http://127.0.0.1:12345 http://www.espn.com/
curl -v -x http://127.0.0.1:12345 http://www.nba.com/
echo Proxy log:
tail ../logs/proxy.log

echo Testing HTTP CONNECT
curl -v -x http://127.0.0.1:12345 https://www.nba.com/
curl -v -x http://127.0.0.1:12345 https://www.espn.com/
echo Proxy log:
tail ../logs/proxy.log

echo Testing POST requests
curl -v -x http://127.0.0.1:12345 -u farice:farice23 http://texnotes.me/login/
echo Proxy log:
tail ../logs/proxy.log


echo Testing hits in the cache
curl -v -x http://127.0.0.1:12345 http://www.example.com/
curl -v -x http://127.0.0.1:12345 http://www.example.com/
curl -v -x http://127.0.0.1:12345 http://www.example.com/
echo Proxy log:
tail ../logs/proxy.log
