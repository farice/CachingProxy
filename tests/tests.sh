#!/usr/bin/env bash
echo Proxy server is up

echo Testing basic HTTP...
curl -v -x http://vcm-3262.vm.duke.edu:12345 http://www.example.com/
echo Testing concurret HTTP...
curl -v -x http://vcm-3262.vm.duke.edu:12345 http://www.example.com/ &
curl -v -x http://vcm-3262.vm.duke.edu:12345 http://www.espn.com/ &
curl -v -x http://vcm-3262.vm.duke.edu:12345 http://www.nba.com/ &
echo Testing HTTP CONNECT
curl -v -x http://vcm-3262.vm.duke.edu:12345 https://www.nba.com/ &
curl -v -x http://vcm-3262.vm.duke.edu:12345 https://www.espn.com/ &
echo Testing POST requests
curl -u farice:farice23 http://texnotes.me/login/
echo Testing hits in the cache
curl -v -x http://vcm-3262.vm.duke.edu:12345 http://www.example.com/
curl -v -x http://vcm-3262.vm.duke.edu:12345 http://www.example.com/
curl -v -x http://vcm-3262.vm.duke.edu:12345 http://www.example.com/
echo Proxy log:
cat ../logs/proxy.log
