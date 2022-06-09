# Warpdrive

## Developing with Docker

Warpdrive for Linux currently builds only on CentOS 7.
There is a `Dockerfile` based on CentOS 7 image containing all the dependencies needed for building and developing Warpdrive.

1. First you need to build the Dockerfile:

```bash
docker build -t warpdrive .
```

2. Then you can start a container binding volumes for `warpdrive`, `flightsql-odbc` and `arrow` repos:

```bash
docker run -it \
  -v /PATH_TO_LOCAL_WARPDRIVE_REPO:/opt/warpdrive \
  -v /PATH_TO_LOCAL_FLIGHTSQL_ODBC_REPO:/opt/flightsql-odbc \
  -v /PATH_TO_LOCAL_ARROW_REPO:/opt/arrow \
  --rm --name warpdrive \
  --add-host=host.docker.internal:host-gateway \
  warpdrive:latest bash
```

**NOTE:** Be sure to replace `PATH_TO_LOCAL_..._REPO` to the actual paths on your computer.

3. To build Warpdrive inside of this container, simply run `./build.sh`.

The build will generate a binary `/opt/warpdrive/_build/release/libarrow-odbc.so.` which is the driver to be loaded on ODBC tools such as odbcinst and iodbc.
