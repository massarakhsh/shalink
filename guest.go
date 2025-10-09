package shalink

import (
	"fmt"
	"net"
	"time"

	"github.com/massarakhsh/lik/log"
)

type ItGuest struct {
	ItTask

	connect    *ItConnect
	addr       net.UDPAddr
	toWrite    chan *ItChunk
	lastActive time.Time
}

func BuildClient(connect *ItConnect, addr net.UDPAddr) *ItGuest {
	client := &ItGuest{connect: connect, addr: addr}
	client.toWrite = make(chan *ItChunk, 1024)
	client.lastActive = time.Now()
	go client.goRun()
	return client
}

func (guest *ItGuest) goRun() {
	guest.isStarting = true
	for !guest.isStoping {
		select {
		case chunk := <-guest.toWrite:
			guest.send(chunk)
		case <-time.After(time.Millisecond):
		}
	}
	guest.isStarting = false
	guest.close()
}

func (guest *ItGuest) send(chunk *ItChunk) {
	data := chunk.ToCode()
	fmt.Printf("To guest send: %s\n", string(chunk.data))
	for tr := 0; tr < 2; tr++ {
		if rsz, err := guest.connect.conn.WriteTo(data, &guest.addr); rsz == len(data) && err == nil {
			break
		} else {
			log.SayInfo("Error WriteTo server: sz=%d, rsz=%d, err=%s", len(data), rsz, fmt.Sprint(err))
			time.Sleep(time.Microsecond * 10)
		}
	}
}

func (guest *ItGuest) close() {
	guest.gate.Lock()
	defer guest.gate.Unlock()

	guest.isStoping = true
	if guest.toWrite != nil {
		close(guest.toWrite)
		guest.toWrite = nil
	}
}
