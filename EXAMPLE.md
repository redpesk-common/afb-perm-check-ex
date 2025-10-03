
# Example of use of permission check extension

The permission check extension, named `PREM-CHECK` is intended
to bring the permission check feature of AFB framework to clients
that are not inserted in the framework but that trust to use it
for permission checking.

So let explore that example.

## Description of the example

An edge equipement is running a on a MCU too smal to
handle permission logic. That equipement is securely connected
using a trusted connection to a server that can check the
permissions using cynagora service.

In that example, the MCU leverage afb-zephyr items that natively
integrates redpesk framework permission checking. That native
permission check is compiled in a such way that instead of calling
directly cynagora service calls a redpesk API for checking the
permission.

Because redpesk APIs can be distribute across machines, the
effective service of permission check is done on the remote
machine by querying cynagora service and by transfering the
answer to the original requester.

Below figure shows that:

```
Zephyr                               (TLS)
application        afb-zephyr       NETWORK         afb-binder  afb-perm-check-ext   cynagora
    :                 :               / /               :               :                :
    :  1.check perm   :               \ \               :               :                :
    :---------------->: 2.perm/check  / /               :               :                :
    :                 :---------------\ \ 2.perm/check  :               :                :
    :                 :               / /-------------->:   3.process   :                :
    :                 :               \ \               :-------------->:   4.check      :
    :                 :               \ \               :               :--------------->:
    :                 :               / /               :               :<---------------:
    :                 :               / /               :<--------------:                :
    :                 :               \ \<--------------:               :
    :                 :<--------------/ /               :
    :<----------------:               \ \
```

In detail:

1. The original query is either: (a) an automatic query resulting from
   the declaration of a required permission for a verb, or, (b) an
   explict query performed using framework API (`afb_req_check_permission`)

2. When the framework library afb-zephyr receives the request to check the
   permission, it translates the request to a call to the API `perm`, VERB
   `check`. The API is implemented as a remote service call through a TLS
   connection to the service running in a binder.

3. The request is received and processed by the loadable binder extension
   `afb-perm-check-ext`.

4. The extension use `afb-libafb` internals to check the permission using
   `cynagora`.

The remaining of this document explains four things:

1. What are redpesk permissions, how it is used and what it expects.

2. Compiling the zephyr example

3. The setting up of the example.

4. Testing and playing with the example

## Dealing with redpesk permissions

Within redpesk, check of permissions implies that the client is trusted.
When running on redpesk, the client is identified by its SMACK label
that is guaranteed by Linux kernel.

When the client runs on a remote platform, it can not be trusted using SMACK
label. Instead, the client must identify itself with a token and that token
must be validated by the permission framework cynagora. This is achieved by checking
that the permission `urn:redpesk:token:valid` is granted to the token used as cynagora
session. In other word, a client presenting the token *TOKEN* is trusted if a cynagora
rule `* TOKEN * urn:redpesk:token:valid yes` exists.

When the client is trusted, the permissions are checked using the same principles.
When the client runs on the same redpesk OS, the SMACK label can be used.
Otherwise, the token is used as session for checking the permissions.


## Compiling the zephyr example

The project [afb-zephyr-samples](https://github.com/redpesk-samples/afb-zephyr-samples)
contains sample applications for integration of redpeskframework and zephyr OS.

The example **remote-io-control** when compiled with `CHECK_PERM` defined to be 1
(see file `remote-io-control/src/main.c` line 63) adds feature to check the client permission
when the client calls the verb `login`.

Instead of using DNS, the example use a fixed IP address. That address is
controled by the macro `PREM_CHECK_IP` and can be set near line 70 of
file `remote-io-control/src/main.c`

The compilation zephyr samples is described in README files.
It requires the zephyr module [afb-zephyr](https://github.com/redpesk-core/afb-zephyr)


## Setting up the example

That section helps to setup: cynagora service, permission checker service,
zephyr example.

### Start cynagora daemon

The permission manager cynagora runs as a server. That server can be launched using command `cynagorad`.

The command `cynagorad` is provided by [redpesk SDK](
https://docs.redpesk.bzh/docs/en/master/getting_started/host-configuration/docs/0-build-microservice-overview.html).
It can also be compiled from [sources](https://github.com/redpesk-core/sec-cynagora).
In all cases, it also provides the command `cynagora-admin` used below.

To start it, either call `cynagorad` or, for dumping activity, `cynagorad log on`.


### Add the permissions required by the demo

When cynagora service runs (`cynagorad`), the permissions can be added
using the admin command `cynagora-admin`.

```
cynagora-admin set '*' CANREAD  '*' urn:redpesk:token:valid yes
cynagora-admin set '*' CANWRITE '*' urn:redpesk:token:valid yes
cynagora-admin set '*' CANWRITE '*' urn:redpesk:zephyr:partner:writer yes
```

The session serves as security token so the client identifier and the user identifier
can be anything. That is the meaning of the stars. Take care that stars might be expanded
by the shell to the list of files in the current directory and then must be in quotes.


### Start the permission checker service

The command `afb-binder` is the redpesk binder, it is provided by [redpesk SDK](
https://docs.redpesk.bzh/docs/en/master/getting_started/host-configuration/docs/0-build-microservice-overview.html).

The service that checks permissions is here run in a separate instance, showing that
the management of permissions can be on an instance different of the client instance.

The permission checker runs as a binder extension. It must be compiled from
the present project. It is loaded in the binder using the option `--extension`.

By default, it provides the API *perm* with the single verb *check*. That API is
exported in RPC on TCP port 4444 using the option '--rpc-server'.

```
afb-binder -p 3333 --extension libafb-perm-check-ext.so --rpc-server tcp:*:4444/perm
```

### Start zephyr

Once you have compiled zephyr example (see above), you should flash
it to the target board (using `west flash`).

Then to sart zephyr, simply reset or boot the board.


## Testing and playing the example

Because today, the program `afb-client` does not support
RPC protocol, it is needed to start a redpesk binder that
serves as relay between the command line tool `afb-client`
and zephyr application.

### Start the relay binder, client of zephyr

The relay to zephyr application is done using the command below:

```
afb-binder --port 1111 --rpc-client=tcp:XXXXXX:1234/zephyr
```

Where `XXXXXX` is the IP of the board.

This command tells to export as API zephyr the API replying at RPC
protocol on port 1234 of IP XXXXXX.

The binder then serves using protocol WSAPI at port 1111 and forward to
the board at `tcp:XXXXXX:1234` the calls to API **zephyr**.

### Test and play the example

To play with the example, we use the program `afb-client` that connects
to the relay binder. This is done using the command below:

```
afb-client -t TOKEN localhost:1111/api
```

Where `TOKEN` is the security token that presents the client.
That token is used when checking permissions.

So to play the example, we should use the predefined tokens
`CANREAD` for being allowed to read, or, `CANWRITE` for being
allowed to write, or, any other value (like `CANT`) that can not
do anything.

For example:

- without permissions: `afb-client -t CANT localhost:1111/api`
- can read:            `afb-client -t CANREAD localhost:1111/api`
- can read and write:  `afb-client -t CANWRITE localhost:1111/api`

Then once connected, `afb-client` expects command inputs from the user.
The commands are of the form: `api verb arg`. So we will try the following
commands and see want happens:

```
zephyr login
zephyr state
zephyr led toggle
zephyr logoff
```

The verb `state` require to be trusted. It just need the permission `urn:redpesk:token:valid`.

The verb `led` with the argument `toggle` requires to be trusted
(permission `urn:redpesk:token:valid`) and to have the permission
`urn:redpesk:zephyr:partner:writer`.

The test of permission `urn:redpesk:zephyr:partner:writer` is done when the user
calls `login`.


## Exercice

The connection between zephyr application and permission checker service
isn't secured because it is done without any protection. It is not difficult
to use a TLS connection instead.

How would do it using AFB settings?

Useful links:

- afb-binder manual https://docs.redpesk.bzh/docs/en/master/redpesk-os/afb-binder/afb-binder.1.html
- call-extern-api example https://github.com/redpesk-samples/afb-zephyr-samples/tree/master/call-extern-api
