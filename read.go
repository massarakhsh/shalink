package shalink

import (
	"fmt"
	"net"
	"time"
)

const maxUdpSize = 65536

func (link *Link) goReading() {
	buffer := make([]byte, maxUdpSize) // Максимальный размер UDP пакета
	for !link.isStoping {
		if conn := link.conn; conn != nil {
			var size int
			var client *Client
			if link.IsServer() {
				n, clientAddr, err := conn.ReadFromUDP(buffer)
				if err != nil {
					fmt.Println("Error reading:", err)
					continue
				}
				client = link.findClient(clientAddr, true)
				if link.terminal.config.IsMirror && client != nil {
					//fmt.Printf("Mirror data: %d\n", n)
					data := buffer[:n]
					link.conn.WriteToUDP(data, &client.addr)
				} else {
					size = n
				}
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
					chunk.client = client
					link.terminal.pushInChunk(chunk)
				}
			}
		} else {
			time.Sleep(timeoutRun)
		}
	}
	link.terminal.SayLog("Reading stoped")
}

func (link *Link) findClient(addr *net.UDPAddr, create bool) *Client {
	link.clientsGate.Lock()
	defer link.clientsGate.Unlock()

	for client := link.clientsPool.first; client != nil; client = client.next {
		if client.addr.IP.Equal(addr.IP) && client.addr.Port == addr.Port {
			client.lastInAt = time.Now()
			return client
		}
	}

	if !create {
		return nil
	}

	client := createClient(*addr)
	link.clientsPool.insertClient(client)

	return client
}
