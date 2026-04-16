package shalink

import (
	"sync"
	"time"
)

const countSize = 10

type Count struct {
	gate      sync.Mutex
	frequency bool
	at        int64
	size      int
	values    [countSize]float64
}

func (count *Count) Inc() {
	count.Add(1)
}

func (count *Count) Add(val float64) {
	count.gate.Lock()
	defer count.gate.Unlock()

	count.prepare(true)
	count.values[0] += val
}

func (count *Count) Set(val float64) {
	count.gate.Lock()
	defer count.gate.Unlock()

	count.prepare(false)
	count.values[0] = val
}

func (count *Count) Get() float64 {
	if count.size <= 1 {
		return float64(count.values[0])
	}
	count.gate.Lock()
	defer count.gate.Unlock()

	count.prepare(count.frequency)
	sum := float64(0)
	for i := 1; i < count.size; i++ {
		sum += count.values[i]
	}
	return float64(sum) / float64(count.size-1)
}

func (count *Count) prepare(frequency bool) {
	count.frequency = frequency
	now := time.Now().Unix()
	if count.size == 0 {
		count.at = now
		count.size = 1
	}
	delta := now - count.at
	if delta < 0 {
		return
	}
	if delta > 0 {
		for to := countSize - 1; to >= 0; to-- {
			from := to - int(delta)
			if from >= 0 {
				count.values[to] = count.values[from]
			} else if frequency {
				count.values[to] = 0
			} else if to > 0 {
				count.values[to] = count.values[0]
			}
		}
		count.at = now
		count.size += int(delta)
		if count.size > countSize {
			count.size = countSize
		}
	}
}
