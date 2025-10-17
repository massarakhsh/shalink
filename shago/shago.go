package shago

/*
#include "../src/terminal.h"
*/

import (
	"C"
	"time"

	shalink "github.com/massarakhsh/shalink/go"
)

type ItTerminal struct {
	terminal *shalink.ItTerminal
}

func BuildTerminal(name string) *ItTerminal {
	c_name := C.CString(name)
	terminal := shalink.BuildTerminal(name)
	C.free(c_name)
	return &ItTerminal{terminal: terminal}
}

func (terminal *ItTerminal) GetName() string {
	return terminal.terminal.GetName()
}

func (terminal *ItTerminal) AddConnect(addr string, isServer bool) {
	terminal.terminal.AddConnect(addr, isServer)
}

func (terminal *ItTerminal) SendData(channel int, data []byte) {
	terminal.terminal.SendData(channel, data)
}

func (terminal *ItTerminal) GetData(wait time.Duration) (int, int64, []uint8) {
	return terminal.terminal.GetData(wait)
}

func (terminal *ItTerminal) Stop() {
	terminal.terminal.Stop()
}

func (terminal *ItTerminal) IsStoping() bool {
	return terminal.terminal.IsStoping()
}
