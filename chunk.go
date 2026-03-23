package shalink

import (
	"encoding/binary"
	"time"
	"unsafe"
)

const chunkProtoPass = 0x0
const chunkProtoSync = 0x1
const chunkProtoRequest = 0x3
const chunkProtoData = 0x10

const dtgMaxSize int = 1420
const chunkHeadSize int = int(unsafe.Sizeof(chunkHead{})) // 20
const chunkInfoSize int = dtgMaxSize - chunkHeadSize

//go:packed
type chunkHead struct {
	bell         uint8
	channel      uint8
	sizeData     uint16
	indexChunk   uint32
	indexPacket  uint32
	offsetPacket uint32
	sizePacket   uint32
}

type chunkData struct {
	head chunkHead
	data [chunkInfoSize]uint8

	createdAt time.Time
	liveTo    time.Time
	Used      int
	predChunk *chunkData
	nextChunk *chunkData
}

func chunkFromBytes(data []byte) *chunkData {
	size := len(data)
	if size < chunkHeadSize {
		return nil
	}
	chunk := &chunkData{}
	chunk.head.bell = uint8(data[0])
	chunk.head.channel = uint8(data[1])
	chunk.head.sizeData = binary.BigEndian.Uint16(data[2:4])
	chunk.head.indexChunk = binary.BigEndian.Uint32(data[4:8])
	chunk.head.indexPacket = binary.BigEndian.Uint32(data[8:12])
	chunk.head.offsetPacket = binary.BigEndian.Uint32(data[12:16])
	chunk.head.sizePacket = binary.BigEndian.Uint32(data[16:20])
	sizeData := size - chunkHeadSize
	if sizeData != int(chunk.head.sizeData) {
		return nil
	}
	if sizeData > chunkInfoSize {
		return nil
	}
	copy(chunk.data[:], data[chunkHeadSize:size])
	return chunk
}

func (chunk *chunkData) chunkToBytes() []byte {
	data := []byte{}
	data = append(data, chunk.head.bell)
	data = append(data, chunk.head.channel)
	data = binary.BigEndian.AppendUint16(data, chunk.head.sizeData)
	data = binary.BigEndian.AppendUint32(data, chunk.head.indexChunk)
	data = binary.BigEndian.AppendUint32(data, chunk.head.indexPacket)
	data = binary.BigEndian.AppendUint32(data, chunk.head.offsetPacket)
	data = binary.BigEndian.AppendUint32(data, chunk.head.sizePacket)
	if chunk.head.sizeData > 0 {
		data = append(data, chunk.data[:chunk.head.sizeData]...)
	}
	return data
}
