package shalink

import (
	"encoding/binary"
)

type Synchro struct {
	chunkOutCount uint32
	chunkOutFirst uint32
	chunkOutLast  uint32
	holePairs     uint32
	holeList      [maxHoleCount * 2]uint32
}

const maxHoleCount = 100
const synchroHeadSize int = 16

func synchroFromBytes(data []byte) Synchro {
	size := len(data)
	var synch Synchro
	if size < synchroHeadSize {
		return synch
	}
	synch.chunkOutCount = binary.BigEndian.Uint32(data[0:4])
	synch.chunkOutFirst = binary.BigEndian.Uint32(data[4:8])
	synch.chunkOutLast = binary.BigEndian.Uint32(data[8:12])
	holePairs := binary.BigEndian.Uint32(data[12:16])
	if size != synchroHeadSize+int(holePairs)*8 {
		return synch
	}
	if holePairs > maxHoleCount {
		holePairs = maxHoleCount
	}
	synch.holePairs = holePairs
	for i := 0; i < int(holePairs); i++ {
		synch.holeList[i*2] = binary.BigEndian.Uint32(data[16+i*8 : 20+i*8])
		synch.holeList[i*2+1] = binary.BigEndian.Uint32(data[20+i*8 : 24+i*8])
	}
	return synch
}

func synchroToBytes(synch Synchro) []byte {
	data := []byte{}
	data = binary.BigEndian.AppendUint32(data, synch.chunkOutCount)
	data = binary.BigEndian.AppendUint32(data, synch.chunkOutFirst)
	data = binary.BigEndian.AppendUint32(data, synch.chunkOutLast)
	holePairs := synch.holePairs
	if holePairs > maxHoleCount {
		holePairs = maxHoleCount
	}
	data = binary.BigEndian.AppendUint32(data, holePairs)
	for i := 0; i < int(holePairs); i++ {
		data = binary.BigEndian.AppendUint32(data, synch.holeList[i*2])
		data = binary.BigEndian.AppendUint32(data, synch.holeList[i*2+1])
	}
	return data
}
