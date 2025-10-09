package shalink

import (
	"sync"
)

type ItTask struct {
	gate       sync.RWMutex
	isStarting bool
	isStoping  bool
}

func (task *ItTask) Stop() {
	task.isStoping = true
}

func (task *ItTask) IsStarting() bool {
	return task.isStarting
}

func (task *ItTask) IsStoping() bool {
	return task.isStoping
}
