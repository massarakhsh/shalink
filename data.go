package shalink

import (
	"time"
)

const timeoutRun = time.Millisecond * 10
const idxLow uint32 = 0x40000000
const idxHi uint32 = 0xc0000000

type Task struct {
	isStarted bool
	isStoping bool
	isStoped  bool
}

func uint32Compare(a uint32, b uint32) int {
	if a == b {
		return 0
	}
	delta := a - b
	if delta <= idxLow {
		return 1
	}
	if delta >= idxHi {
		return -1
	}
	return 2
}
