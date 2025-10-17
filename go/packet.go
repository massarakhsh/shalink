package shalink

type ItPacket struct {
	channel int
	index   int64
	data    []byte
}

func BuildPacket(channel int, index int64, data []byte) *ItPacket {
	packet := &ItPacket{channel: channel, index: index, data: data}
	return packet
}

func (packet *ItPacket) GetChannel() int {
	return packet.channel
}

func (packet *ItPacket) GetIndex() int64 {
	return packet.index
}
func (packet *ItPacket) GetData() []byte {
	return packet.data
}
