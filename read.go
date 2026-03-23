package shalink

import (
	"fmt"
	"net"
	"time"

	"github.com/massarakhsh/lik/log"
)

const maxUdpSize = 65536

func (link *Link) goReading() {
	buffer := make([]byte, maxUdpSize) // Максимальный размер UDP пакета
	for !link.isStoping {
		if conn := link.conn; conn != nil {
			var size int
			var income *Income
			if link.isServer {
				n, clientAddr, err := conn.ReadFromUDP(buffer)
				if err != nil {
					fmt.Println("Error reading:", err)
					continue
				}
				income = link.findIncome(clientAddr, true)
				size = n
			} else {
				n, err := conn.Read(buffer)
				if err != nil {
					fmt.Println("Error reading:", err)
					continue
				}
				size = n
			}
			if size > 0 {
				if chunk := chunkFromBytes(buffer[:size]); chunk != nil {
					_ = income
					link.terminal.receiveChunk(chunk)
				}
			}
		} else {
			time.Sleep(timeoutRun)
		}
	}
	log.SayInfo("Reading stoped")
}

func (link *Link) findIncome(addr *net.UDPAddr, create bool) *Income {
	link.incomeGate.Lock()
	defer link.incomeGate.Unlock()

	for income := link.incomeFirst; income != nil; income = income.incomeNext {
		if income.addr.IP.Equal(addr.IP) && income.addr.Port == addr.Port {
			return income
		}
	}

	if !create {
		return nil
	}

	income := createIncome(*addr)
	link.insertIncome(income)
	income.start()

	return income
}
