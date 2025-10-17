package shalink

import "time"

type ItChunk struct {
	proto   int
	channel int
	index   int64
	offset  int
	data    []uint8

	createAt  time.Time
	maskSend  uint
	maskRecv  uint
	predChunk *ItChunk
	nextChunk *ItChunk
}

type ItChunkCode struct {
	proto   [1]uint8
	size    [3]uint8
	channel [4]uint8
	index   [8]uint8
	offset  [4]uint8
	data    []uint8
}

const ChunkSizeHead = 20
const MaxOutDTG = 80
const MaxInDTG = 1500
const ChunkSizeInfo = MaxOutDTG - ChunkSizeHead

const ProtoKeep = 0
const ProtoPacket = 8

func BuildChunk(proto int, channel int, index int64, offset int) *ItChunk {
	chunk := &ItChunk{}
	chunk.createAt = time.Time{}
	chunk.proto = proto
	chunk.channel = channel
	chunk.index = index
	chunk.offset = offset
	return chunk
}

func CodeToChunk(code []byte) *ItChunk {
	size := len(code) - ChunkSizeHead
	if size < 0 {
		return nil
	}
	sz := 0
	for b := range 3 {
		sz += int(code[1+b]) << (b * 8)
	}
	if sz != size {
		return nil
	}
	var proto int
	for b := range 1 {
		proto += int(code[b]) << (b * 8)
	}
	var channel int
	for b := range 4 {
		channel += int(code[4+b]) << (b * 8)
	}
	var index int64
	for b := range 8 {
		index += int64(code[8+b]) << (b * 8)
	}
	var offset int
	for b := range 4 {
		offset += int(code[16+b]) << (b * 8)
	}
	chunk := BuildChunk(proto, channel, index, offset)
	data := make([]uint8, size)
	copy(data, code[ChunkSizeHead:])
	chunk.data = data
	return chunk
}

func (chunk *ItChunk) ToCode() []byte {
	size := len(chunk.data)
	code := make([]byte, ChunkSizeHead+size)
	for b := range 1 {
		code[b] = byte((chunk.proto >> (b * 8)) & 0xff)
	}
	for b := range 3 {
		code[1+b] = byte((size >> (b * 8)) & 0xff)
	}
	for b := range 4 {
		code[4+b] = byte((chunk.channel >> (b * 8)) & 0xff)
	}
	for b := range 8 {
		code[8+b] = byte((chunk.index >> (b * 8)) & 0xff)
	}
	for b := range 4 {
		code[16+b] = byte((chunk.offset >> (b * 8)) & 0xff)
	}
	copy(code[ChunkSizeHead:], chunk.data)
	return code
}
