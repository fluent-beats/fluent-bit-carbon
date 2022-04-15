# Fluent Bit Plugin

The following repository provides the structure and build-system helpers to develop C dynamic plugins for [Fluent Bit](https://fluentbit.io) like _inputs_, _filters_ and _outputs_.

>  Fluent Bit API development/usage is out of the scope of this article.

## Requirements

- Docker
- Docker image `etriphany/fluent-bit-plugin-dev`

## Getting Started
```bash
./build.sh
```

## Test
```bash
 docker run --rm -v $(pwd)/build:/my_plugin fluent/fluent-bit:1.8.4 /fluent-bit/bin/fluent-bit -e /my_plugin/flb-in_carbon.so -i carbon -o stdout
 ```