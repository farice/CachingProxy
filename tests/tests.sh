#!/usr/bin/env bash
echo Proxy server is up

echo Testing basic HTTP...
curl -v -x http://vcm-3262.vm.duke.edu:12345 https://www.example.com/
echo Proxy log:
cat ../logs/proxy.log
