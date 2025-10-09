package main

import (
	"fmt"
	"time"

	"github.com/massarakhsh/lik"
	"github.com/massarakhsh/lik/log"
	"github.com/massarakhsh/shalink"
)

var addrServer = "127.0.0.1:8891"

func main() {
	var listClients []*shalink.ItTerminal

	server := startServer("SRV")
	for nc := range 1 {
		name := fmt.Sprintf("CLI%d", nc)
		client := startClient(name)
		listClients = append(listClients, client)
	}

	time.Sleep(time.Second * 5)

	log.SayInfo("Stoping ...")
	server.Stop()
	for _, client := range listClients {
		client.Stop()
	}
	time.Sleep(time.Second * 5)
	log.SayInfo("Done")
}

func startServer(name string) *shalink.ItTerminal {
	log.SayInfo("Start server %s", name)
	terminal := shalink.BuildTerminal(name)
	terminal.AddConnect(addrServer, true)
	go func(terminal *shalink.ItTerminal) {
		for number := 1; number < 10 && !terminal.IsStoping(); number++ {
			time.Sleep(time.Second * 2)
			set := lik.BuildSet()
			count := 12
			set.SetValue("count", count)
			for n := range count {
				key := fmt.Sprintf("n%d", n)
				set.SetValue(key, n)
			}
			msg := set.Serialize()
			//log.SayInfo("Server %s send packet %s channel=%d", name, msg, 1)
			terminal.SendData(1, []byte(msg))
		}

		log.SayInfo("Terminated server %s", name)
	}(terminal)
	return terminal
}

func startClient(name string) *shalink.ItTerminal {
	log.SayInfo("Start client %s", name)
	terminal := shalink.BuildTerminal(name)
	terminal.AddConnect(addrServer, false)
	terminal.IsStarting()
	go func(terminal *shalink.ItTerminal) {
		for !terminal.IsStoping() {
			if packet := terminal.GetPacket(time.Millisecond * 100); packet != nil {
				msg := string(packet.GetData())
				fmt.Printf("Client %s got packet %s channel=%d index=%d\n", name, string(packet.GetData()), packet.GetChannel(), packet.GetIndex())
				if set := lik.SetFromString(msg); set == nil {
					fmt.Printf("Client %s ERROR: do not parsing\n", name)
				} else if count := set.GetInt("count"); count <= 0 {
					fmt.Printf("Client %s ERROR: bad count\n", name)
				} else {
					for n := range count {
						key := fmt.Sprintf("n%d", n)
						if set.GetInt(key) != n {
							fmt.Printf("Client %s ERROR: bad value %d\n", name, n)
							break
						}
					}
				}
			}
		}
		log.SayInfo("Terminated client %s", name)
	}(terminal)
	return terminal
}
