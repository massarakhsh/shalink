package shalink

import (
	"fmt"
	"time"
)

type Statistic struct {
	OutPacketsCount Count
	OutChunksData   Count
	OutChunksRetry  Count
	OutChunksTotal  Count
	OutBytesData    Count
	OutBytesTotal   Count

	InPacketsCount Count
	InPacketsReady Count

	InChunksData  Count
	InChunksTotal Count
	InBytesData   Count
	InBytesTotal  Count

	InPacketsQueue Count
	OutChunksQueue Count
	InChunksQueue  Count

	DebugPackets int64
	DebugChunks  int64
	DebugBytes   int64
	ChunksLost   int64

	Formula string
	Logs    []string
}

func (terminal *Terminal) GetStatistic() *Statistic {
	terminal.statistic.Formula = terminal.getFormula()
	terminal.statistic.InPacketsQueue.Set(float64(terminal.inPackets.getCount()))
	terminal.statistic.InChunksQueue.Set(float64(terminal.inChunks.getCount()))
	terminal.statistic.OutChunksQueue.Set(float64(terminal.outChunks.getCount()))
	chunks := chunkCountUsed - terminal.outChunks.getCount() - terminal.inChunks.getCount()
	terminal.statistic.ChunksLost = int64(chunks)
	return &terminal.statistic
}

func (terminal *Terminal) getFormula() string {
	formula := ""
	terminal.gateLinks.Lock()
	for link := terminal.links.first; link != nil; link = link.linkNext {
		formula += " + "
		if link.config.IsServer {
			formula += "Server"
		} else {
			formula += "Client"
		}
		if cnt := link.clientsPool.count; cnt > 0 {
			formula += fmt.Sprintf("(%d)", cnt)
		}
	}
	terminal.gateLinks.Unlock()
	return formula
}

func (stat *Statistic) sayLog(format string, args ...any) {
	log := fmt.Sprintf("[%s] %s", time.Now().Format("02-01-2006 15:04:05"), fmt.Sprintf(format, args...))
	logs := append(stat.Logs, log)
	if size := len(logs); size > 4 {
		logs = logs[size-4:]
	}
	stat.Logs = logs
}
