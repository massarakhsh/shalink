package shalink

import (
	"net"
	"time"

	"github.com/massarakhsh/lik/log"
)

type Income struct {
	Task

	addr net.UDPAddr

	link       *Link
	incomeNext *Income
	incomePred *Income

	chanSend chan *Chunk
}

func createIncome(addr net.UDPAddr) *Income {
	//log.SayInfo("Creating income: %s", addr.String())
	income := &Income{addr: addr}
	income.chanSend = make(chan *Chunk, 1024)
	return income
}

func (income *Income) stop() {
	income.isStoping = true
}

func (income *Income) start() {
	go income.goRun()
}

func (income *Income) goRun() {
	income.isStarted = true
	for !income.isStoping {
		select {
		case chunk, ok := <-income.chanSend:
			if !ok {
				break
			}
			income.sendChunk(chunk)
		case <-time.After(timeoutRun):
		}
	}
	income.isStoped = true
	log.SayInfo("Income stopped")
}

func (income *Income) pushChunks(chunks []*Chunk) {
	for _, chunk := range chunks {
		income.pushChunk(chunk)
	}
}

func (income *Income) pushChunk(chunk *Chunk) {
	income.chanSend <- chunk
}

func (income *Income) sendChunk(chunk *Chunk) {
	if data := chunkToBytes(chunk); data != nil {
		income.link.conn.WriteToUDP(data, &income.addr)
	}
}
