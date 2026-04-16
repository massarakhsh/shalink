package shalink

import (
	"net"
	"time"
)

type Client struct {
	addr     net.UDPAddr
	lastInAt time.Time
	synch    Synchro

	next *Client
	pred *Client
}

type clientPool struct {
	count int
	first *Client
	last  *Client
}

func createClient(addr net.UDPAddr) *Client {
	client := &Client{addr: addr}
	client.lastInAt = time.Now()
	return client
}

func (pool *clientPool) insertClient(client *Client) {
	client.pred = pool.last
	if client.pred == nil {
		pool.first = client
	} else {
		client.pred.next = client
	}
	pool.last = client
	pool.count++
}

func (pool *clientPool) extractClient(client *Client) {
	pred := client.pred
	next := client.next
	if pred != nil {
		pred.next = next
	} else {
		pool.first = next
	}
	if next != nil {
		next.pred = pred
	} else {
		pool.last = pred
	}
	pool.count--
}
