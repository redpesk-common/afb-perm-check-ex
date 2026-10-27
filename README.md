# Permission Checking API extension for libafb

AFB extension for querying cynagora permissions

## Usage

Example of use within `afb-binder`:

```bash
afb-binder --name=perm-check --verbose \
   --extension=libafb-perm-check-ext.so \
   --perm-check-scope=public
```

## Dependencies

Depends on [afb-libafb](https://github.com/redpesk-core/afb-libafb) and [afb-binding](https://github.com/redpesk-core/afb-binding).

The simplest is to install microservice development packages,
see [Build framework on your computer]({% chapter_link host-build-doc.build-framework-on-your-computer %}).

## Build from source

```bash
mkdir build
cd build
cmake ..
make
```

## Internals

The extension defines an API implementing one verb for checking cynagora permissions.

The verb can receive either four strings, one JSON object, or one JSON array.

When a JSON object is given, it must include at least the fields
'client', 'user', 'session' and 'permission'.

When four strings are given, or a JSON array of four values, the values are
positional and must be in that order: 'client', 'user', 'session' and 'permission'.

Examples:

- single value: `{"client":"C","user":"U","session":"S","permission":"P"}`
- single value: `["C","U","S","P"]`
- four values:
  - value 1: `C`
  - value 2: `U`
  - value 3: `S`
  - value 4: `P`

The verb replies an integer value:

- 1 when the permission is granted
- 0 when the permission is refused
- a negative value if an error occurred

On error, the permission is not granted.

## Settings

Default settings are generally good enough.

The default names are `perm` for the API and `check` for the verb.
These names can be changed using command line argument:

- To change the API name:  `--perm-check-api=NAME`
- To change the verb name:  `--perm-check-verb=NAME`

The API is created in the default scope. But the scope can be changed using the
option `--perm-check-scope=NAME`.

The API can be disabled using the option `--perm-check-disabled`.

Public use:

  * -a, --perm-check-api=NAME: sets the API name (default perm)
  * -d, --perm-check-disabled: disables the extension
  * --perm-check-scope=NAME: sets scope of the declared API
  * --perm-check-verb=NAME: sets the VERB name (default check)

