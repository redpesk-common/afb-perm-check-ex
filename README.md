# Permission Checking API extension for libafb

AFB micro-service for querying cynagora permissions

## Usage

```bash
afb-binder --name=perm-check --verbose \
   --extension=libafb-perm-check-ext.so \
   --perm-check-scope=public
```

## Dependencies

* afb micro-service development rpm. <https://docs.redpesk.bzh/docs/en/master/getting_started/host-configuration/docs/1-Setup-your-build-host.html>

## Build from source

```bash
mkdir build
cd build
cmake ..
make
```
