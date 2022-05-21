# Description

[Fluent Bit](https://fluentbit.io) input plugin to support metrics using [StatsD](https://github.com/statsd/statsd) metrics + [Graphite/Carbon](https://graphite.readthedocs.io/en/latest/tags.html) tags

# Requirements

- Docker
- Docker image `etriphany/fluent-bit-plugin-dev`

# Build
```bash
./build.sh
```

# Test
```bash
docker run --rm -p 8125:8125/udp -v $(pwd)/build:/my_plugin fluent/fluent-bit:1.8.4 /fluent-bit/bin/fluent-bit -e /my_plugin/flb-in_carbon.so -i carbon -o stdout
 ```

# Design

## Tags support
The original `StatsD` packet format looks like this:
```
bucket:metric|type|@sample_rate
```
They can be combined to produce the following metrics:
- Counter
- Sampler
- Timmer
- Gauge
- Sets

The problem with `StatsD` packets is that you can't apply sharding to your metrics,
like group them by environment or service for example. They are classified as flat metrics because of this limitation.

In order shard our metrics, we need to extended the packets to support custom metadata, and thats why we need the tags.

The extended packet format looks like this:
```
bucket;tag1=value1;tag2=value2:metric|type|@sample_rate
```
These packets can be used the same way original `StatsD` packets but now we'll be able to shard the metrics.

The plugin limits the max number of tags to be `5`, with is enough for most of scenarios.

## Extendend metric namespacing
Properly naming your metrics is critical to avoid conflicts, confusing data and potentially wrong interpretation later on.

This plugin enforces using extended namespacing, using the same `bucket` name conventions presented on [Practical Guide to Graphite Monitoring](https://matt.aimonetti.net/posts/2013-06-practical-guide-to-graphite-monitoring/):

The `bucket` string can use the following format:
```
<namespace>.<section>.<target>.<action>
```
Internally the plugin will detect and parse each one properly.

## Extended packet examples
### Counter
```
accounts.authentication;env=prod;service=my-service:1|c
```
### Sampler
```
my-sampler;env=prod;service=my-service:1|c|@0.1
```
### Timmer
```
query.database.users;env=prod;service=my-service:320|ms|@0.1
```
### Gauge
```
memory.java.heap.used;env=prod;service=my-service:333|g
```
### Set
```
my-set;env=prod;service=my-service:765|s
```