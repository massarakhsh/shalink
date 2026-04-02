package main

import (
	"fmt"
	"time"

	"github.com/massarakhsh/lik"
	"github.com/massarakhsh/lik/log"
	"github.com/massarakhsh/shalink"
	"github.com/spf13/pflag"

	"github.com/gdamore/tcell/v2"
)

var (
	flagUrl     = pflag.String("url", "", "enter point URL")
	flagRole    = pflag.String("role", "local", "role of this instance")
	flagSize    = pflag.Int("size", 1000, "size of each packet")
	flagCount   = pflag.Int("count", 10, "number of packets to send")
	flagPause   = pflag.Int("pause", 10, "pause between packets in milliseconds")
	flagLatency = pflag.Int("latency", 100, "latency limit in milliseconds")
	flagScreen  = pflag.Bool("screen", false, "display screen")
)

var Version string = "v0.0.0dev"

var parmUrl string
var parmRole string
var parmPacketCount int = 10
var parmPacketSize int = 1000
var parmPacketPause time.Duration = 10 * time.Millisecond
var parmLatency time.Duration = 100 * time.Millisecond
var parmScreen bool

var Screen tcell.Screen
var styleName tcell.Style
var styleData tcell.Style

var isStopping bool
var isPausing bool

func main() {
	log.SayInfo("=== ShaLink test started")
	if !initialize() {
		pflag.PrintDefaults()
		return
	}

	if parmScreen {
		screen, err := tcell.NewScreen()
		if err != nil {
			log.SayInfo("Screen do not initialized")
		} else {
			Screen = screen
			defer closeScreen()
			initScreen()
		}
	}

	config := shalink.ConfigTerminal{Latency: parmLatency}
	if parmRole == "mirror" {
		config.IsMirror = true
	}
	terminal := shalink.CreateTerminal(config)
	//log.SayInfo("Terminal started")
	if lik.RegExCompare(parmRole, "(local|server|mirror)") {
		terminal.AddLink(shalink.ConfigLink{Address: parmUrl, IsServer: true})
		//log.SayInfo("Server started")
	}
	if lik.RegExCompare(parmRole, "(local|client|loop)") {
		terminal.AddLink(shalink.ConfigLink{Address: parmUrl, IsServer: false})
		//log.SayInfo("Client started")
	}
	time.Sleep(1 * time.Second)

	inCount := 0
	outCount := 0
	nextSend := time.Now()
	nextStatistic := time.Now()
	for !isStopping {
		if lik.RegExCompare(parmRole, "(local|server|loop)") && !time.Now().Before(nextSend) {
			if inCount >= parmPacketCount {
				break
			}
			//log.SayInfo("Sending packet %d", inCount)
			data := make([]byte, parmPacketSize)
			roll := inCount % 256
			for i := 0; i < parmPacketSize; i++ {
				data[i] = byte(roll)
				roll = (roll*7 + 1) % 256
			}
			terminal.SendPacket(0, data)
			inCount++
			nextSend = time.Now().Add(parmPacketPause)
		}
		if packet := terminal.ProbePacket(); packet != nil {
			size := len(packet.Data)
			if size > 0 {
				roll := int(packet.Data[0])
				if roll != outCount {
					log.SayInfo("Data ERROR: first byte %d != %d", roll, outCount)
					outCount = roll
				}
				outCount = (outCount + 1) % 256
				for i := 0; i < size; i++ {
					if packet.Data[i] != byte(roll) {
						log.SayInfo("Data ERROR: dep=%d", i)
						break
					}
					roll = (roll*7 + 1) % 256
				}
			}
			//log.SayInfo("Received packet")
		}
		if Screen != nil && time.Now().After(nextStatistic) {
			showStatistic(terminal)
			nextStatistic = time.Now().Add(time.Second)
		}
		time.Sleep(time.Millisecond)
	}
	isStopping = true
	time.Sleep(time.Second)
	showStatistic(terminal)
	if Screen != nil && isPausing {
		Screen.PollEvent()
	}
}

func initialize() bool {
	pflag.Parse()

	parmRole = *flagRole
	fmt.Printf("--role=%s\n", parmRole)
	if !lik.RegExCompare(parmRole, "(local|server|client|loop|mirror)") {
		fmt.Printf("role %s MUST by (local|server|client|loop|mirror)\n", parmRole)
		return false
	}
	parmUrl = *flagUrl
	fmt.Printf("--url=%s\n", parmUrl)
	if parmUrl == "" {
		fmt.Printf("url MUST BY present\n")
		return false
	}
	parmPacketSize = *flagSize
	fmt.Printf("--size=%d\n", parmPacketSize)
	parmPacketCount = *flagCount
	fmt.Printf("--count=%d\n", parmPacketCount)
	parmPacketPause = time.Duration(*flagPause) * time.Millisecond
	fmt.Printf("--pause=%d milliseconds\n", parmPacketPause/time.Millisecond)
	parmLatency = time.Duration(*flagLatency) * time.Millisecond
	fmt.Printf("--latency=%d milliseconds\n", parmLatency/time.Millisecond)
	parmScreen = *flagScreen
	fmt.Printf("--screen=%t\n", parmScreen)
	return true
}

func initScreen() {
	Screen.Init()
	Screen.Clear()
	styleName = tcell.StyleDefault.Foreground(tcell.ColorGray)
	styleData = tcell.StyleDefault.Foreground(tcell.ColorWhite)

	go func() {
		for !isStopping {
			if ev := Screen.PollEvent(); ev != nil {
				switch evt := ev.(type) {
				case *tcell.EventKey:
					if evt.Key() == tcell.KeyEscape || evt.Key() == tcell.KeyCtrlC {
					}
					isPausing = true
					isStopping = true
				case *tcell.EventResize:
					Screen.Sync()
				}
			}
		}
	}()
}

func closeScreen() {
	Screen.Fini()
	Screen = nil
	fmt.Print("\nStop\n")
}

func showStatistic(terminal *shalink.Terminal) {
	statistic := terminal.GetStatistic()
	ln := 1
	opc := statistic.OutPacketCount.GetValue("")
	ipc := statistic.InPacketCount.GetValue("")
	showPairFloat(0, ln, "Out packets", opc, "/s")
	showPairFloat(40, ln, "In packets", ipc, "/s")
	ln++
	opr := statistic.OutPacketReady.GetValue("")
	ipr := statistic.InPacketReady.GetValue("")
	showPairFloatPerc(0, ln, "Out packets ready", opr, opc, "/s")
	showPairFloatPerc(40, ln, "In packets ready", ipr, ipc, "/s")
	ln++
	showPairFloat(40, ln, "In packets queue", statistic.InPacketQueue.GetValue(""), "")
	ln++
	occ := statistic.OutChunkCount.GetValue("")
	icc := statistic.InChunkCount.GetValue("")
	showPairFloat(0, ln, "Out chunks", occ, "/s")
	showPairFloat(40, ln, "In chunks", icc, "/s")
	ln++
	showPairFloat(0, ln, "Out chunks queue", statistic.OutChunkQueue.GetValue(""), "")
	showPairFloat(40, ln, "In chunks queue", statistic.InChunkQueue.GetValue(""), "")
	ln++
	if isPausing {
		ln++
		showText(0, ln, "Press any key to exit", true)
	}
	if Screen != nil {
		Screen.Show()
	}
}

func showPairFloat(x, y int, name string, val float64, sfx string) {
	tval := valToString(val) + sfx
	showPair(x+20, y, name, tval+"  ")
}

func showPairFloatPerc(x, y int, name string, val float64, aval float64, sfx string) {
	tval := valToString(val) + sfx
	showPair(x+20, y, name, tval+"  ")
	if val > aval {
		tval += " (100%)"
	} else if aval > 0 {
		tval += fmt.Sprintf(" (%.1f%%)", val*100/aval)
	}
	showPair(x+20, y, name, tval+"  ")
}

func valToString(val float64) string {
	if val >= 1000000 {
		return fmt.Sprintf("%.1fM", val/1000000)
	} else if val >= 1000 {
		return fmt.Sprintf("%.1fK", val/1000)
	} else if val >= 10 {
		return fmt.Sprintf("%.1f", val)
	} else if val >= 1 {
		return fmt.Sprintf("%.2f", val)
	} else if val != 0 {
		return fmt.Sprintf("%f", val)
	} else {
		return "0"
	}
}

func showPair(x, y int, name string, data string) {
	showText(x, y, name, false)
	showText(x+20, y, data, true)
}

func showText(x, y int, text string, isData bool) {
	if Screen != nil {
		var style tcell.Style
		if isData {
			style = styleData
		} else {
			style = styleName
		}
		for i, r := range text {
			Screen.SetContent(x+i, y, r, nil, style)
		}
	} else {
		fmt.Print(text)
		if !isData {
			fmt.Print(": ")
		} else {
			fmt.Println()
		}
	}
}
