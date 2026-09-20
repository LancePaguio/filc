# Build

## For windows, linux, and macOS

```
g++ -std=c++17 -O2 -Isrc src/*.cpp -o filc
```

## Using Make to build filc
NOTE: For Linux and macOS only

```
make install
```

# Usage

Windows only
```
filc.exe [OPTIONS] [FILEPATH]
```

Linux and macOS only
```
./filc [OPTIONS] [FILEPATH]
```

With make install
```
filc [OPTIONS] [FILEPATH]
```

# Check with this options first

```
./filc --help
```

# Examples

```
filc --time test/sample_1.test
```
NOTE: the file extension here isn't restrictive
you can test it with any file extension even
with a file with no extensions

# Makefile options

```
make
```
NOTE: this only builds it without going into /local/bin
so you have to run it like this every time `./filc`

```
make debug
```
NOTE: this is the unoptimized build

```
make clean
```
NOTE: this only deletes the filc file not the object and dependency files

```
make uninstall
```
NOTE: this rm -f the filc inside your local bin
