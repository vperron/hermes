Hermes
======

Receive, stack, deliver, persist streams of wireless traces.


Architecture
------------

*hermes* uses an [ZeroMQ](http://www.zeromq.org)-powered Actor Model to dispatch the incoming message to
various agents.

### Commands between agents

Agents communicate between themselves using a set of predefinite commands.

```
agent-req = request

request =  ( postmsg / postidmsg / posthashmsg / sendhashmsg / postfile )

postmsg = 'POSTMSG' serialized*
postfile = 'POSTFILE' msg_id filename
postidmsg = 'POSTIDMSG' msg_id serialized*
posthashmsg = 'POSTHASHMSG' partial_hash serialized
```

Agents can send requests of this form to each other.

```
agent-rep = response

response =  ( success / failure )

success = 'SUCCESS' msg_id
failure = 'FAILURE' msg_id
```

The Agent receiving a success or failure answer can then take the appropriate decision.


### Receiver agent

Receives wireless messages from other software. Is also the `main` method of hermes.

#### Parameters

* hermes.receiver.has_ftp (default: true)
* hermes.receiver.endpoint (default: tcp://\*:10070)
* hermes.receiver.backup (default: false)
* hermes.receiver.realtime (default: false)

#### Logic

1. Receives multipart messages in the form:

```
incoming = timestamp mac_addr_hash mac_addr_prefix dBm [ssid]
```

2. Calculate a hash from everything except timestamp.

```
partial_hash = str(SuperFastHash(mac_addr_hash + mac_addr_prefix + dBm + ssid))
```

3. Serialize the whole message into a single byte array.

```
serialized = [json | csv | msgpack](timestamp, mac_addr_hash, mac_addr_prefix, dBm, ssid)
```

4. Send messages to various agents.

```
if (persist):
  persist_agent.send( create_post_msg(serialized) )
if (realtime):
  http_agent.send( create_post_msg(serialized) )
else:
  hashtable_agent.send( create_post_hash_msg(hashtable, serialized) )
```

### Persist agent

This agent has a very, very simple task. It receives messages (POST or SEND), serializes them
in an append-only fashion into a file, and sends back error/success status if needed.

It is also able to gzip the current log file and rename it to something else after a predefined size
is reached.

#### Parameters

* hermes.persist.path (default: /root/persist/)
* hermes.persist.maxrecords (default: 1000)

### Hashtable agent

This agent is used when realtime upload is not activated, which is the case by default.
It gets POSTHASHMSG messages from the Receiver agent, and maintains an in-memory hashtable.
This enables to throttle very efficiently the measures: during the configurable lifetime of 
the hashtable, we make sure we don't get two times exactly the same measure.
When some trigger is fired, it POSTs the whole hashtable to the http_agent, and creates a new one.

#### Parameters

* hermes.hashtable.maxsize (default: 1000)
* hermes.hashtable.maxtime (default: 60s)

#### Logic

1. Receives single-line POSTHASHMSG in the form:

```
incoming = 'POSTHASHMSG' partial_hash serialized
```

2. Insert `serialized` into `hashtable` unless `partial\_hash` key already exists.

```
unless hashtable.has_key(partial_hash):
  hashtable.insert(partial_hash, serialized)
```

3. Check if the `hashtable` has reached its maximum size, or time since last creation is reached.

```
if hashtable.length() > hermes.hashtable.maxsize or now() > hashtable.creation.date():
  msg = POSTMSG
  for key, value in hashtable:
    msg.append(value)
  http_agent.send(msg)
  hashtable.flush()
  hashtable.creation.date = now()
```

### HTTP Agent

This agent is responsible for trying to send every single record it gets to the HTTP endpoint.
In case of failure, it sends the data to the Failure agent, with or without a msg_id.

#### Parameters

* hermes.http.server (default: http://server_name
* hermes.http.auth_endpoint (default: /auth-token/)
* hermes.http.data_endpoint (default: /datastreams/DATASTREAM_ID/datapoints/)
* hermes.http.username
* hermes.http.password
* hermes.http.timeout (default: 15s)

#### Logic

```
incoming = [POSTIDMSG msg_id | POSTMSG] serialized*

# Message comes from the failure agent already
if (incoming.msgtype == POSTIDMSG):
  msg_id = incoming.msg_id
  bulk = generate_collection(incoming.serialized*)
  if !http.send(bulk):
    failure_agent.send( create_error_msg(msg_id) )

# Message comes from somebody else: try and maybe send it back to failure agent as-is
if (incoming.msgtype == POSTMSG):
  bulk = generate_collection(incoming.serialized*)
  if !http.send(bulk):
    failure_agent.send( bulk )
```

### Failure Agent

That agent receives POST (long) serialized strings from HTTP Agent.
It gives that string an ID (incremental), and stores the string in a gzipped file and keeps
track of the time of that save.
When some time has elapsed, the file is sent again to HTTP agent.

#### Parameters

hermes.failure.path (default: /tmp/failure/)
hermes.failure.maxsize (default: 1000)
hermes.failure.retry_time (default: 15min)
hermes.failure.max_retry (default: 10)

#### Logic

```
class FileStruct = { name, retries } 
class FileTable = Array<FileStruct>()

file_table = new FileTable()

while True:
  incoming = recv()
  if (incoming.msgtype == POSTMSG ):
    file_id = str(SuperFastHash(incoming.serialized))
    file_name = hermes.failure.path + file_id + '.json.gz'
    gzipWrite (file_name, incoming.serialized)
    fileStruct = new FileStruct(file, 0)
    file_table.add(file_id, fileStruct)
  if (incoming.msgtype == SUCCESS):
    delete_file(file_table.lookup(incoming.msg_id).name)
    file_table.delete_record(incoming.msg_id)
  if (incoming.msgtype == ERROR):
    file_table[incoming.msg_id].retries++

  for id, file in file_table:
    if file.retries > hermes.failure.max_retry: 
      ftp_agent.upload(file)
      file.max_retry = 0 // reset so that we still send via HTTP next time
      continue
    if file.records > hermes.failure.maxsize:
      file.retries++
      http_agent.send( create_post_id_msg(id, file) )
```

### FTP Agent

Tries to upload the file via FTP. Send back SUCCESS or FAILURE to Failure agent in either case.
It receives POSTFILE messages and uses it to try and send the file over FTP.

#### Parameters

* hermes.ftp.endpoint (default: ftp://xxxx:yyyy@server.name)
* hermes.ftp.folder (default: /DATASTREAM_ID/)
* hermes.ftp.timeout (default: 30s)

## Compile

### As an OpenWRT package:

```bash
rm dl/hermes-VERSION.tar.bz2
make package/hermes/install V=99
scp bin/ramips/packages/hermes_VERSION_ramips.ipk root@[remoterouter]:.
```

### Local development install:

```bash
./autogen.sh
./configure --with-xxx=[...]
make
```

Changelog
---------

### 1.0.1

**Date**: 30st August 2013

* Field tested working for 3 months
* Use tokens directly
* Handles standalone mode
* Uses compressed output streams
* Do not receive long server answer

### 1.0.0-alpha

**Date**: 21st April 2013

* Rewrite everything
* Actor model
* Throttle data

### 0.2.2

**Date**: 10th April 2013

* Major rewrite of the message processing chain
* Full e2é test of decision chain
* Unit tests
* Many, many bugfixes



## License

MIT
