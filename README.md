# Permission Checking API extension for libafb

AFB extension for querying cynagora permissions

## Usage

Exemple of use within `afb-binder`:

```bash
afb-binder --name=perm-check --verbose \
   --extension=libafb-perm-check-ext.so \
   --perm-check-scope=public
```

## Dependencies

Depends of [afb-libafb](https://github.com/redpesk-core/afb-libafb) and [afb-binding](https://github.com/redpesk-core/afb-binding).

The simplest is to install micro service development packages,
see [afb micro-service development](https://docs.redpesk.bzh/docs/en/master/getting_started/host-configuration/docs/1-Setup-your-build-host.html).

## Build from source

```bash
mkdir build
cd build
cmake ..
make
```

## Internals

The extension defines an API implementing one verb for checking cynagora permissions.

The verb can receive either 4 strings, one JSON object, or, one JSON array.

When a JSON object is given, it must have at least the 4 fields
'client', 'user', 'session' and 'permission'.

When 4 strings are given or a JSON array of 4 values, the values are
positionnal and must be in that order: 'client', 'user', 'session' and 'permission'.

Examples:

- single value: `{"client":"C","user":"U","session":"S","permission":"P"}`
- single value: `["C","U","S","P"]`
- 4 values:
  - value 1: `C`
  - value 2: `U`
  - value 3: `S`
  - value 4: `P`

The verb reply an integer value:

- 1 when the permission is granted
- 0 when the permission is refused
- a negative value if an error occured

On error, the permission is not granted.

## Settings

Default settings are generally well enough.

The default names are `perm` for the API and `check` for the verb.
These names can be changed using command line argument:

- To change the API name:  `--perm-check-api=NAME`
- To change the verb name:  `--perm-check-verb=NAME`

The API is created in the default scope. But the scope can be changed using the
option `--perm-check-scope=NAME`.

iThe API can be disabled using the option `--perm-check-disabled`.

public using

  -a, --perm-check-api=NAME  Set the API name (default perm)
  -d, --perm-check-disabled  Disable the extension
      --perm-check-scope=NAME   Set scope of the declared API
      --perm-check-verb=NAME Set the VERB name (default check)

