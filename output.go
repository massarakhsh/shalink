package shalink

import "time"

func (terminal *ItTerminal) outputPushPacket(packet *ItPacket) {
	chunks := terminal.outputExpandChunks(packet)
	terminal.outputPushChunks(chunks)
}

func (terminal *ItTerminal) outputExpandChunks(packet *ItPacket) []*ItChunk {
	var chunks []*ItChunk
	size := len(packet.data)
	for dep := 0; dep < size; {
		pot := size - dep
		if pot > ChunkSizeInfo {
			pot = ChunkSizeInfo
		}
		chunk := &ItChunk{}
		chunk.createAt = time.Time{}
		chunk.proto = ProtoPacket
		chunk.channel = packet.channel
		chunk.index = packet.index
		chunk.offset = dep
		chunk.data = packet.data[dep : dep+pot]
		if pred := terminal.outputLastChunk; pred != nil {
			chunk.predChunk = pred
			pred.nextChunk = chunk
		} else {
			terminal.outputFirstChunk = chunk
		}
		terminal.outputLastChunk = chunk
		chunks = append(chunks, chunk)
		dep += pot
	}

	return chunks
}

func (terminal *ItTerminal) outputPushChunks(chunks []*ItChunk) {
	for _, connect := range terminal.listConnects {
		if connect.IsStarting() {
			connect.PushChunks(chunks)
		}
	}
}

func (terminal *ItTerminal) purgeChunk(chunk *ItChunk) {
	pred := chunk.predChunk
	next := chunk.nextChunk
	if pred != nil {
		pred.nextChunk = next
		chunk.predChunk = nil
	} else {
		terminal.outputFirstChunk = next
	}
	if next != nil {
		next.predChunk = pred
		chunk.nextChunk = nil
	} else {
		terminal.outputLastChunk = pred
	}
}
