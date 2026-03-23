package shalink

type sendPacket struct {
	channel int
	index   uint32
	data    []byte
}
