package shalink

import (
	"net"
	"sync"
	"time"

	"github.com/massarakhsh/lik/log"
)

type Link struct {
	Task

	address    string
	isServer   bool
	udpAddr    net.UDPAddr
	nextOpenAt time.Time

	conn *net.UDPConn

	terminal *Terminal
	linkNext *Link
	linkPred *Link

	incomeGate  sync.RWMutex
	incomeFirst *Income
	incomeLast  *Income

	chanSend chan *chunkData
}

const timeoutOpen = time.Second * 5

func createLink(address string, isServer bool) *Link {
	link := &Link{address: address, isServer: isServer}
	link.chanSend = make(chan *chunkData, 1024)
	return link
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
	}
	link.isStoped = true
	log.SayInfo("Link stopped")
}

func (link *Link) insertIncome(income *Income) {
	income.link = link
	income.incomePred = link.incomeLast
	if income.incomePred == nil {
		link.incomeFirst = income
	} else {
		income.incomePred.incomeNext = income
	}
	link.incomeLast = income
}

func (link *Link) extractIncome(income *Income) {
	pred := income.incomePred
	next := income.incomeNext
	if pred == nil {
		link.incomeFirst = next
	} else {
		pred.incomeNext = next
	}
	if next == nil {
		link.incomeLast = pred
	} else {
		next.incomePred = pred
	}
	income.link = nil
}

func (link *Link) pushChunks(chunks []chunkData) {
	for _, chunk := range chunks {
		link.chanSend <- &chunk
	}
}

func (link *Link) sendChunk(chunk *chunkData) {
	log.SayInfo("Link sending data: %d bytes", chunk.head.sizeData)

	if !link.IsOpened() {
		log.SayInfo("Link is not opened, cannot send data")
	} else if link.isServer {
		link.incomeGate.Lock()
		for income := link.incomeFirst; income != nil; income = income.incomeNext {
			income.pushChunk(chunk)
		}
		link.incomeGate.Unlock()
	} else {
		if datas := chunk.chunkToBytes(); datas != nil {
			link.conn.Write(datas)
		}
	}
}
