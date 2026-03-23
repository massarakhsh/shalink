package main

import (
	"time"

	"github.com/massarakhsh/lik/log"
	"gitlab.com/massarakhsh/shalink"
)

var Version string = "v0.0.0dev"

var parmPacketCount int = 5
var parmPacketSize int = 1000
var parmPacketPause time.Duration = 1000 * time.Millisecond

var isStopping bool

func main() {
	log.SayInfo("=== ShaLink test started")

	terminal := shalink.CreateTerminal()
	terminal.AddListener(":8901")
	terminal.AddConnection("localhost:8901")
	log.SayInfo("Terminal started")
	time.Sleep(1 * time.Second)

	for p := 0; !isStopping && p < parmPacketCount; p++ {
		log.SayInfo("Sending packet %d", p)
		data := make([]byte, parmPacketSize)
		for i := 0; i < parmPacketSize; i++ {
			data[i] = byte(i % 256)
		}
		terminal.SendData(0, data)
		time.Sleep(parmPacketPause)
	}
	isStopping = true
}
