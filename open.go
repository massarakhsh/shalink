package shalink

import (
	"net"

	"github.com/massarakhsh/lik/log"
)

func (link *Link) open() {
	if link.IsOpened() {
		return
	}
	link.openResolve()
	if link.IsServer() {
		link.openServer()
	} else {
		link.openClient()
	}
	if link.IsOpened() {
		go link.goReading()
	}
}

func (link *Link) openServer() {
	conn, err := net.ListenUDP("udp", &link.udpAddr)
	if err != nil {
		return
	}
	link.conn = conn
	log.SayInfo("UDP server listening on : %d", link.udpAddr.Port)

	// buffer := make([]byte, 65536) // Максимальный размер UDP пакета
	// for {
	//     n, clientAddr, err := conn.ReadFromUDP(buffer)
	//     if err != nil {
	//         fmt.Println("Error reading:", err)
	//         continue
	//     }

	//     fmt.Printf("Received %d bytes from %s: %s\n",
	//         n, clientAddr, string(buffer[:n]))

	//     // Отправляем ответ
	//     response := []byte("UDP message received\n")
	//     conn.WriteToUDP(response, clientAddr)
	// }
}

func (link *Link) openClient() {
	conn, err := net.DialUDP("udp", nil, &link.udpAddr)
	if err != nil {
		return
	}

	link.conn = conn
	log.SayInfo("UDP client connected to : %s", link.udpAddr.String())
}

func (link *Link) openResolve() {
	if addr, err := net.ResolveUDPAddr("udp", link.config.Address); err != nil {
		link.udpAddr = net.UDPAddr{}
	} else {
		link.udpAddr = *addr
		if link.IsServer() {
			addr.IP = net.ParseIP("0.0.0.0")
		}
	}
}
