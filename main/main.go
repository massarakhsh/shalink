package main

import (
	"fmt"
	"time"

	"github.com/massarakhsh/lik"
	"github.com/massarakhsh/lik/log"
	"github.com/massarakhsh/shalink/shago"
)

var addrServer = "127.0.0.1:8891"
var isStoping = false

func main() {
	var listClients []*shago.ItTerminal

	server := startServer("SRV")
	for nc := range 1 {
		name := fmt.Sprintf("CLI%d", nc)
		client := startClient(name)
		listClients = append(listClients, client)
	}

	startAt := time.Now()
	for time.Since(startAt) < 30*time.Second && !isStoping {
		time.Sleep(time.Millisecond)
	}

	log.SayInfo("Stoping ...")
	isStoping = true
	server.Stop()
	for _, client := range listClients {
		client.Stop()
	}
	time.Sleep(time.Second)
	log.SayInfo("Done")
}

func startServer(name string) *shago.ItTerminal {
	log.SayInfo("Start server %s", name)
	terminal := shago.BuildTerminal(name)
	terminal.AddConnect(addrServer, true)
	go func(terminal *shago.ItTerminal) {
		for number := 1; number < 10 && !isStoping && !terminal.IsStoping(); number++ {
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

func startClient(name string) *shago.ItTerminal {
	log.SayInfo("Start client %s", name)
	terminal := shago.BuildTerminal(name)
	terminal.AddConnect(addrServer, false)
	go func(terminal *shago.ItTerminal) {
		for !isStoping && !terminal.IsStoping() {
			if channel, index, data := terminal.GetData(time.Millisecond * 100); index > 0 {
				msg := string(data)
				fmt.Printf("Client %s got packet %s channel=%d index=%d\n", name, msg, channel, index)
				if set := lik.SetFromString(msg); set == nil {
					fmt.Printf("Client %s ERROR: do not parsing\n", name)
					isStoping = true
				} else if count := set.GetInt("count"); count <= 0 {
					fmt.Printf("Client %s ERROR: bad count\n", name)
					isStoping = true
				} else {
					for n := range count {
						key := fmt.Sprintf("n%d", n)
						if set.GetInt(key) != n {
							fmt.Printf("Client %s ERROR: bad value %d\n", name, n)
							isStoping = true
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
