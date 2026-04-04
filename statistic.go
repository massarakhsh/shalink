package shalink

import (
	"github.com/massarakhsh/lik/metric"
)

type Statistic struct {
	OutPacketCount metric.MetricValue
	OutPacketReady metric.MetricValue

	OutChunkCount metric.MetricValue
	OutChunkQueue metric.MetricValue
	OutChunkSynch metric.MetricValue

	InPacketCount metric.MetricValue
	InPacketQueue metric.MetricValue
	InPacketReady metric.MetricValue

	InChunkCount metric.MetricValue
	InChunkQueue metric.MetricValue

	PacketAlloc metric.MetricValue
	ChunkAlloc  metric.MetricValue
}

func (terminal *Terminal) GetStatistic() *Statistic {
	terminal.statistic.PacketAlloc.SetValueInt("", int64(packetAllocate))
	terminal.statistic.ChunkAlloc.SetValueInt("", int64(chunkAllocate))
	return &terminal.statistic
}
