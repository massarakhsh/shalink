package shalink

import (
	"math/rand"
	"net"
	"sync"
	"time"
)

type ConfigLink struct {
	Address  string
	IsServer bool
}

type Link struct {
	Task
	config ConfigLink

	udpAddr      net.UDPAddr
	nextOpenAt   time.Time
	nextControls time.Time

	conn      *net.UDPConn
	lastWrite time.Time

	terminal *Terminal
	linkNext *Link
	linkPred *Link

	clientsGate sync.RWMutex
	clientsPool clientPool

	chanSend chan *Chunk
}

type linkPool struct {
	first *Link
	last  *Link
}

const timeoutOpen = time.Second * 5
const timeoutWrite = time.Microsecond * 10
const timeoutClient = time.Second * 15

func createLink(terminal *Terminal, config ConfigLink) *Link {
	link := &Link{terminal: terminal, config: config}
	link.chanSend = make(chan *Chunk, 1024)
	return link
}

func (pool *linkPool) initialize() {
}

func (pool *linkPool) insert(link *Link) {
	link.linkPred = pool.last
	if link.linkPred == nil {
		pool.first = link
	} else {
		link.linkPred.linkNext = link
	}
	pool.last = link
}

func (pool *linkPool) extract(link *Link) {
	pred := link.linkPred
	next := link.linkNext
	if pred == nil {
		pool.first = next
	} else {
		pred.linkNext = next
	}
	if next == nil {
		pool.last = pred
	} else {
		next.linkPred = pred
	}
	link.terminal = nil
}

func (link *Link) IsServer() bool {
	return link.config.IsServer
}

func (link *Link) IsOpened() bool {
	return link.conn != nil
}

func (link *Link) stop() {
	link.isStoping = true
}

func (link *Link) start() {
	go link.goRun()
}

func (link *Link) goRun() {
	link.isStarted = true
	for !link.isStoping {
		if link.IsOpened() {
			select {
			case chunk, ok := <-link.chanSend:
				if !ok {
					break
				}
				link.sendChunk(chunk)
			case <-time.After(timeoutRun):
			}
		} else if time.Now().After(link.nextOpenAt) {
			link.nextOpenAt = time.Now().Add(timeoutOpen)
			link.open()
		} else {
			time.Sleep(timeoutRun)
		}
		if time.Now().After(link.nextControls) {
			if link.IsServer() {
				link.controlClients()
			}
			link.nextControls = time.Now().Add(timeoutClient * 100 / time.Duration(90+rand.Intn(20)))
		}
	}
	link.isStoped = true
	link.terminal.SayLog("Link stopped")
}

func (link *Link) pushChunks(chunks []*Chunk) {
	for _, chunk := range chunks {
		link.chanSend <- chunk
	}
}

func (link *Link) sendChunk(chunk *Chunk) {
	if link.IsOpened() {
		data := chunkToBytes(chunk)
		if link.IsServer() {
			link.sendDataServer(data)
		} else {
			link.sendDataClient(data, nil)
		}
	}
	chunk.Free()
}

func (link *Link) sendDataServer(data []byte) {
	link.clientsGate.Lock()
	defer link.clientsGate.Unlock()

	for client := link.clientsPool.first; client != nil; client = client.next {
		link.sendDataClient(data, client)
	}
}

func (link *Link) sendDataClient(data []byte, client *Client) {
	if pau := time.Since(link.lastWrite); pau < timeoutWrite {
		time.Sleep(timeoutWrite - pau)
	}
	link.terminal.statistic.OutChunksTotal.Inc()
	link.terminal.statistic.OutBytesTotal.Add(float64(len(data)))
	if rand.Float64() < dropFrequency {
		return
	}
	if client == nil {
		link.conn.Write(data)
	} else {
		link.conn.WriteToUDP(data, &client.addr)
	}
}

func (link *Link) controlClients() {
	link.clientsGate.Lock()
	defer link.clientsGate.Unlock()

	for client := link.clientsPool.first; client != nil; {
		next := client.next
		if time.Since(client.lastInAt) >= timeoutClient {
			link.clientsPool.extractClient(client)
		}
		client = next
	}
}
