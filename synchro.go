package shalink

import "encoding/binary"

type Synchro struct {
	chunkCount uint32
	chunkFirst uint32
	chunkLast  uint32
	res0       uint32
}

func synchroFromBytes(data []byte) Synchro {
	size := len(data)
	var synch Synchro
	if size < 16 {
		return synch
	}
	synch.chunkCount = binary.BigEndian.Uint32(data[0:4])
	synch.chunkFirst = binary.BigEndian.Uint32(data[4:8])
	synch.chunkLast = binary.BigEndian.Uint32(data[8:12])
	synch.res0 = binary.BigEndian.Uint32(data[12:16])
	return synch
}

func synchroToBytes(synch Synchro) []byte {
	data := []byte{}
	data = binary.BigEndian.AppendUint32(data, synch.chunkCount)
	data = binary.BigEndian.AppendUint32(data, synch.chunkFirst)
	data = binary.BigEndian.AppendUint32(data, synch.chunkLast)
	data = binary.BigEndian.AppendUint32(data, synch.res0)
	return data
}

func (terminal *Terminal) goSynchronize() {
	if count := terminal.outChunks.count; count > 0 {
		first := terminal.outChunks.first
		last := terminal.outChunks.last
		if first != nil && last != nil {
			indexFirst := first.head.indexChunk
			indexLast := last.head.indexChunk
			if uint32Compare(indexFirst, indexLast) <= 0 {
				var synch Synchro
				synch.chunkCount = uint32(count)
				synch.chunkFirst = uint32(indexFirst)
				synch.chunkLast = uint32(indexLast)
				//synch.res0 = uint32(indexLast)
				terminal.outSynchroSend(synch)
			}
		}
	}
}
