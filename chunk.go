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

type Chunk struct {
	head chunkHead
	data [chunkInfoSize]uint8

	createdAt time.Time
	liveTo    time.Time
	predChunk *Chunk
	nextChunk *Chunk
}

//go:packed
type chunkHead struct {
	proto        uint8
	channel      uint8
	sizeData     uint16
	indexChunk   uint32
	indexPacket  uint32
	offsetPacket uint32
	sizePacket   uint32
}

type PoolChunk struct {
	count int
	first *Chunk
	last  *Chunk
}

func allocChunk() *Chunk {
	chunk := &Chunk{}
	if chunk == nil {
		panic("chunk do not allocated")
	}
	chunk.createdAt = time.Now()
	return chunk
}

func freeChunk(chunk *Chunk) {
}

func chunkFromBytes(data []byte) *Chunk {
	size := len(data)
	if size < chunkHeadSize {
		return nil
	}
	chunk := allocChunk()
	chunk.head.proto = uint8(data[0])
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

func (chunk *Chunk) chunkToBytes() []byte {
	data := []byte{}
	data = append(data, chunk.head.proto)
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

func (pool *PoolChunk) insertIn(pred *Chunk, chunk *Chunk, next *Chunk) {
	chunk.predChunk = pred
	if pred != nil {
		pred.nextChunk = chunk
	} else {
		pool.first = chunk
	}
	chunk.nextChunk = next
	if next != nil {
		next.predChunk = chunk
	} else {
		pool.last = chunk
	}
	pool.count++
}

func (pool *PoolChunk) insertLast(chunk *Chunk) {
	pool.insertIn(pool.last, chunk, nil)
}

func (pool *PoolChunk) extract(chunk *Chunk) {
	pred := chunk.predChunk
	next := chunk.nextChunk
	if pred != nil {
		pred.nextChunk = next
	} else {
		pool.first = next
	}
	if next != nil {
		next.predChunk = pred
	} else {
		pool.last = pred
	}
	pool.count--
}
