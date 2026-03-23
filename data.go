package shalink

import (
	"time"
)

const timeoutRun = time.Millisecond * 1000

type Task struct {
	isStarted bool
	isStoping bool
	isStoped  bool
}
