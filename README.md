# shalink

## Testing

`shalink [options] [address]:port`

- -h - get help page
- -s - start sender (default as listening server)
- -r - start receiver (default as connecting client)
- -i - use invert mode (sender as client and receiver as server)
- -l msec - set latency, default 100
- -z size - set size of every packets, default 10000
- -n count - set count of packets, default 100
- -p msec - set pause between packets, default 10
- address:port - domain or ip address and port of UDP entry point, address can be empty

## Examples

`shalink -s -r :9000`

Run sender and receiver on local port 9000 with default parameters. Testing on internal loop, five secunds about.

`shalink -s :9000`

Run sender as server on the localhost, port 9000. Other client must to connect to this port.

`shalink -r 12.34.56.78:9000`

Run receiver from host 12.34.56.78 port 9000.

`shalink -s -i 12.34.56.78:9000`

Run sender as a client that sends data to the server (reverse mode).

`shalink -s -z 1000000 -n 10 -p 200 -l 500 :9000`

Run sender. Server must to send 10 packets by 1000000 bytes with pause 200 msec between packets and latency 500 msec.
