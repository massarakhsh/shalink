package shalink

import (
	"time"
)

type ItTerminal struct {
	ItTask

	name             string
	listConnects     []*ItConnect
	nextMask         uint
	lastControlPools time.Time

	input chan *ItPacket

	outputIndexChannels map[int]int64
	outputFirstChunk    *ItChunk
	outputLastChunk     *ItChunk
}

const periodControlPools = time.Second * 1
const terminalPoolLive = time.Second * 2

func BuildTerminal(name string) *ItTerminal {
	terminal := &ItTerminal{name: name}
	terminal.nextMask = 0x1
	terminal.outputIndexChannels = make(map[int]int64)
	terminal.input = make(chan *ItPacket, 1024)
	go terminal.goRun()
	return terminal
}

func (terminal *ItTerminal) GetName() string {
	return terminal.name
}

func (terminal *ItTerminal) AddConnect(addr string, isServer bool) {
	terminal.gate.Lock()
	defer terminal.gate.Unlock()

	connect := BuildConnect(terminal, addr, terminal.nextMask, isServer)
	terminal.nextMask <<= 1
	terminal.listConnects = append(terminal.listConnects, connect)
}

func (terminal *ItTerminal) SendPacket(packet *ItPacket) {
	terminal.gate.Lock()
	defer terminal.gate.Unlock()

	terminal.outputPushPacket(packet)
}

func (terminal *ItTerminal) SendData(channel int, data []byte) {
	index := terminal.MakeChannelIndex(channel)
	packet := BuildPacket(channel, index, data)
	terminal.SendPacket(packet)
}

func (terminal *ItTerminal) GetPacket(wait time.Duration) *ItPacket {
	select {
	case packet := <-terminal.input:
		return packet
	case <-time.After(wait):
		return nil
	}
}

func (terminal *ItTerminal) goRun() {
	terminal.isStarting = true
	for !terminal.isStoping {
		if time.Since(terminal.lastControlPools) >= periodControlPools {
			go terminal.controlOutput()
			terminal.lastControlPools = time.Now()
		} else {
			time.Sleep(time.Millisecond * 10)
		}
	}
	terminal.isStarting = false
	terminal.close()
}

func (terminal *ItTerminal) MakeChannelIndex(channel int) int64 {
	terminal.gate.Lock()
	defer terminal.gate.Unlock()

	index := terminal.outputIndexChannels[channel]
	index++
	terminal.outputIndexChannels[channel] = index

	return index
}

func (terminal *ItTerminal) controlOutput() {
	terminal.gate.Lock()
	defer terminal.gate.Unlock()

	for {
		chunk := terminal.outputFirstChunk
		if chunk == nil {
			break
		}
		if dura := time.Since(chunk.createAt); dura < terminalPoolLive {
			break
		}
		terminal.purgeChunk(chunk)
	}
}

func (terminal *ItTerminal) close() {
	terminal.gate.Lock()
	defer terminal.gate.Unlock()

	terminal.isStoping = true
	if terminal.input != nil {
		close(terminal.input)
		terminal.input = nil
	}
}
