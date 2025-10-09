package shalink

import "time"

type ItChunk struct {
	proto   int
	channel int
	index   int64
	offset  int
	data    []byte

	createAt  time.Time
	maskSend  uint
	maskRecv  uint
	predChunk *ItChunk
	nextChunk *ItChunk
}

type ItChunkCode struct {
	proto   [1]byte
	size    [3]byte
	channel [4]byte
	index   [8]byte
	offset  [4]byte
	data    []byte
}

const ChunkSizeHead = 20
const MaxSizeDTG = 80 //1420
const ChunkSizeInfo = MaxSizeDTG - ChunkSizeHead

const ProtoKeep = 0
const ProtoPacket = 8

func BuildChunk() *ItChunk {
	return nil
}

func (chunk *ItChunk) ToCode() []byte {
	return DataToCode(chunk.proto, chunk.channel, chunk.offset, chunk.index, chunk.data)
}

func DataToCode(proto int, channel int, offset int, index int64, data []byte) []byte {
	size := len(data)
	code := make([]byte, ChunkSizeHead+size)
	for b := range 1 {
		code[b] = byte((proto >> (b * 8)) & 0xff)
	}
	for b := range 3 {
		code[1+b] = byte((size >> (b * 8)) & 0xff)
	}
	for b := range 4 {
		code[4+b] = byte((channel >> (b * 8)) & 0xff)
	}
	for b := range 8 {
		code[8+b] = byte((index >> (b * 8)) & 0xff)
	}
	for b := range 4 {
		code[16+b] = byte((offset >> (b * 8)) & 0xff)
	}
	for b := range size {
		code[ChunkSizeHead+b] = data[b]
	}
	return code
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
	chunk := &ItChunk{}
	for b := range 1 {
		chunk.proto += int(code[b]) << (b * 8)
	}
	for b := range 4 {
		chunk.channel += int(code[4+b]) << (b * 8)
	}
	for b := range 8 {
		chunk.index += int64(code[8+b]) << (b * 8)
	}
	for b := range 4 {
		chunk.offset += int(code[16+b]) << (b * 8)
	}
	chunk.data = code[ChunkSizeHead:]
	return chunk
}
