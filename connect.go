package shalink

import (
	"fmt"
	"net"
	"time"

	"github.com/massarakhsh/lik/log"
)

type ItConnect struct {
	ItTask

	terminal          *ItTerminal
	addr              string
	isServer          bool
	isOpened          bool
	mask              uint
	conn              *net.UDPConn
	lastActive        time.Time
	lastControlGuests time.Time
	lastReplyKeep     time.Time
	guests            map[string]*ItGuest
	toRead            chan *ItChunk
	toWrite           chan *ItChunk
}

var periodReplyKeep = time.Second * 1
var maxKeepAlive = periodReplyKeep * 5
var periodControlGuests = time.Second

func BuildConnect(terminal *ItTerminal, addr string, mask uint, isServer bool) *ItConnect {
	connect := &ItConnect{terminal: terminal}
	connect.addr = addr
	connect.mask = mask
	connect.isServer = isServer
	connect.guests = make(map[string]*ItGuest)
	connect.toRead = make(chan *ItChunk, 1024)
	connect.toWrite = make(chan *ItChunk, 1024)
	connect.isStarting = true

	go connect.goConnect()
	go connect.goExchange()

	if connect.isServer {
		go connect.goListening()
	} else {
		go connect.goReading()
	}
	return connect
}

func (connect *ItConnect) goExchange() {
	for !connect.isStoping {
		select {
		case chunk := <-connect.toRead:
			connect.receiveChunk(chunk)
		case chunk := <-connect.toWrite:
			connect.sendChunk(chunk)
		case <-time.After(time.Millisecond):
		}
	}
}

func (connect *ItConnect) goConnect() {
	for !connect.isStoping {
		if !connect.isOpened {
			if !connect.open() {
				time.Sleep(time.Millisecond * 100)
			}
		} else if time.Since(connect.lastReplyKeep) >= periodReplyKeep {
			connect.lastReplyKeep = time.Now()
			go connect.replyKeep()
		} else if connect.isServer && time.Since(connect.lastControlGuests) >= periodControlGuests {
			connect.lastControlGuests = time.Now()
			go connect.controlGuests()
		} else {
			time.Sleep(time.Millisecond * 1)
		}
	}
}

func (connect *ItConnect) goReading() {
	data := make([]byte, MaxSizeDTG)

	for !connect.isStoping {
		if !connect.isOpened {
			time.Sleep(time.Millisecond * 1)
		} else if size, err := connect.conn.Read(data); err == nil {
			if chunk := CodeToChunk(data[0:size]); chunk != nil {
				connect.toRead <- chunk
			}
		}
	}
}

func (connect *ItConnect) goListening() {
	data := make([]byte, MaxSizeDTG)

	for !connect.isStoping {
		if !connect.isOpened {
			time.Sleep(time.Millisecond * 1)
		} else if size, addr, err := connect.conn.ReadFromUDP(data); err != nil {
			time.Sleep(time.Millisecond * 1)
		} else if chunk := CodeToChunk(data[0:size]); chunk != nil {
			if guest := connect.seekGuest(addr); guest != nil {
				connect.toRead <- chunk
			}
		}
	}
}

func (connect *ItConnect) controlGuests() {
	connect.gate.Lock()
	defer connect.gate.Unlock()

	for sadr, client := range connect.guests {
		if connect.isStoping || time.Since(client.lastActive) >= maxKeepAlive {
			log.SayInfo("Delete client: %s", client.addr.String())
			close(client.toWrite)
			delete(connect.guests, sadr)
		}
	}
}

func (connect *ItConnect) seekGuest(addr *net.UDPAddr) *ItGuest {
	connect.gate.Lock()
	defer connect.gate.Unlock()

	sAddr := addr.String()
	client := connect.guests[sAddr]
	if client != nil {
		client.lastActive = time.Now()
	} else if client = BuildClient(connect, *addr); client != nil {
		connect.guests[sAddr] = client
		log.SayInfo("Add client: %s", sAddr)
	}

	return client
}

func (connect *ItConnect) replyKeep() {
	data := DataToCode(ProtoKeep, 0, 0, 0, nil)
	//connect.sendChunk()
	connect.conn.Write(data)
}

func (connect *ItConnect) open() bool {
	if connect.conn != nil {
		connect.conn.Close()
		connect.conn = nil
	}

	addr, err := net.ResolveUDPAddr("udp", connect.addr)
	if err != nil {
		return false
	}

	if connect.isServer {
		connect.conn, err = net.ListenUDP("udp", addr)
	} else {
		connect.conn, err = net.DialUDP("udp", nil, addr)
	}

	if connect.conn == nil || err != nil {
		return false
	}

	connect.isOpened = true
	connect.lastActive = time.Now()
	log.SayInfo("Connection opened: %s", connect.addr)

	return true
}

func (connect *ItConnect) PushChunks(chunks []*ItChunk) {
	for _, chunk := range chunks {
		connect.toWrite <- chunk
	}
}

func (connect *ItConnect) receiveChunk(chunk *ItChunk) {
	log.SayInfo("receiveChunk %s", string(chunk.data))
	if chunk.proto == ProtoPacket {
		packet := BuildPacket(chunk.channel, chunk.index, chunk.data)
		connect.terminal.input <- packet
	}
}

func (connect *ItConnect) sendChunk(chunk *ItChunk) {
	log.SayInfo("sendChunk %s", string(chunk.data))
	if !connect.isServer {
		data := chunk.ToCode()
		if sz, err := connect.conn.Write(data); sz == len(data) && err == nil {
		} else {
			log.SayInfo("Error Write server: sz=%d, rsz=%d, err=%s", len(data), sz, fmt.Sprint(err))
			time.Sleep(time.Microsecond * 10)
		}
	} else if (chunk.maskSend & connect.mask) == 0 {
		connect.gate.Lock()
		for _, guest := range connect.guests {
			guest.toWrite <- chunk
		}
		connect.gate.Unlock()
		chunk.maskSend |= connect.mask
	}
}
