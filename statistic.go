package shalink

import (
	"github.com/massarakhsh/lik/metric"
)

type Statistic struct {
	OutPacketCount metric.MetricValue
	OutPacketReady metric.MetricValue

	OutChunkCount metric.MetricValue
	OutChunkQueue metric.MetricValue

	InPacketCount metric.MetricValue
	InPacketQueue metric.MetricValue
	InPacketReady metric.MetricValue

	InChunkCount metric.MetricValue
	InChunkQueue metric.MetricValue
}

func (terminal *Terminal) GetStatistic() Statistic {
	return terminal.statistic
}
