#!/bin/sh

set -e
cd $(dirname $0)/..

docker run --rm \
      -p 8125:8125/udp \
      -v $(pwd)/code/build:/my_plugin \
      fluent/fluent-bit:1.8.4 /fluent-bit/bin/fluent-bit -e /my_plugin/flb-in_carbon.so -i carbon -o stdout