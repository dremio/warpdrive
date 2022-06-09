# ODBC Integration Tests

This folder houses the ODBC integration tests which are forked from the psql library and distributed under the library GPL license.

## Build

```sh
mkdir build && cd build
cmake .. -GNinja
cmake --build .
```

## Running the tests

```bash
# in the root of build
ODBCINI=./odbc.ini DSN=ARROW ./build/%test_name%
```