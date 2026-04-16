package main

import (
	"fmt"
	"math/rand"
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
	flagPause   = pflag.Int("pause", 10, "pause between packets in milliseconds")
	flagLatency = pflag.Int("latency", 100, "latency limit in milliseconds")
	flagScreen  = pflag.Bool("screen", false, "display screen")
)

var Version string = "v0.0.0dev"

var parmUrl string
var parmRole string
var parmPacketSize int = 1000
var parmPacketPause time.Duration = 10 * time.Millisecond
var parmLatency time.Duration = 100 * time.Millisecond
var parmScreen bool

var Screen tcell.Screen
var styleName tcell.Style
var styleData tcell.Style

var isStopping bool
var isPause bool
var isNeed bool
var isFinalize bool

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
	switch parmRole {
	case "local":
		config.IsLocal = true
	case "mirror":
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
		useSend := lik.RegExCompare(parmRole, "(local|server|loop)") && (!isPause || isNeed)
		if useSend && !time.Now().Before(nextSend) {
			isNeed = false
			nextSend = time.Now().Add(parmPacketPause)
			size := 1 + rand.Intn(parmPacketSize)
			data := make([]byte, size)
			roll := inCount % 256
			for i := 0; i < size; i++ {
				data[i] = byte(roll)
				roll = (roll*7 + 1) % 256
			}
			terminal.SendPacket(data)
			inCount++
		} else if packet := terminal.ProbePacket(); packet != nil {
			size := len(packet.Data)
			if size > 0 {
				roll := int(packet.Data[0])
				if roll != outCount {
					if outCount != 0 {
						terminal.SayLog("Data ERROR: first byte %d != %d.  ", roll, outCount)
					}
					outCount = roll
				}
				outCount = (outCount + 1) % 256
				for i := 0; i < size; i++ {
					if packet.Data[i] != byte(roll) {
						terminal.SayLog("Data ERROR: dep=%d.  ", i)
						break
					}
					roll = (roll*7 + 1) % 256
				}
			}
		} else {
			pause := time.Millisecond
			if useSend && time.Until(nextSend) < pause {
				pause = time.Until(nextSend)
			}
			if pause > 0 {
				time.Sleep(pause)
			}
		}
		if Screen != nil && time.Now().After(nextStatistic) {
			showStatistic(terminal)
			nextStatistic = time.Now().Add(time.Second)
		}
	}
	isStopping = true
	time.Sleep(time.Second)
	showStatistic(terminal)
	if Screen != nil && isFinalize {
		Screen.PollEvent()
	}
}

func initialize() bool {
	pflag.Parse()

	parmRole = *flagRole
	fmt.Printf("--role=%s\n\r", parmRole)
	if !lik.RegExCompare(parmRole, "(local|server|client|loop|mirror)") {
		fmt.Printf("role %s MUST by (local|server|client|loop|mirror)\n\r", parmRole)
		return false
	}
	parmUrl = *flagUrl
	fmt.Printf("--url=%s\n\r", parmUrl)
	if parmUrl == "" {
		fmt.Printf("url MUST BY present\n\r")
		return false
	}
	parmPacketSize = *flagSize
	fmt.Printf("--size=%d\n\r", parmPacketSize)
	parmPacketPause = time.Duration(*flagPause) * time.Millisecond
	fmt.Printf("--pause=%d milliseconds\n\r", parmPacketPause/time.Millisecond)
	parmLatency = time.Duration(*flagLatency) * time.Millisecond
	fmt.Printf("--latency=%d milliseconds\n\r", parmLatency/time.Millisecond)
	parmScreen = *flagScreen
	fmt.Printf("--screen=%t\n\r", parmScreen)
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
					switch evt.Key() {
					case tcell.KeyEnter:
						isNeed = true
					case tcell.KeyEscape, tcell.KeyCtrlC:
						isFinalize = true
						isStopping = true
					default:
						switch evt.Rune() {
						case ' ':
							isPause = !isPause
						}
					}
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
	fmt.Print("\n\rStop\n\r")
}

func showStatistic(terminal *shalink.Terminal) {
	statistic := terminal.GetStatistic()
	showText(0, 0, statistic.Formula, true)
	ln := 1
	opc := statistic.OutPacketsCount.Get()
	ipc := statistic.InPacketsCount.Get()
	showPairFloat(0, ln, "Out packets", opc, "/s")
	showPairFloatPerc(40, ln, "In packets", ipc, opc, "/s")
	ln++
	ipr := statistic.InPacketsReady.Get()
	showPairFloatPerc(40, ln, "In packets ready", ipr, opc, "/s")
	ln += 2
	ocd := statistic.OutChunksData.Get()
	icd := statistic.InChunksData.Get()
	showPairFloat(0, ln, "Out chunks data", ocd, "/s")
	showPairFloatPerc(40, ln, "In chunks data", icd, ocd, "/s")
	ln++
	oct := statistic.OutChunksTotal.Get()
	ict := statistic.InChunksTotal.Get()
	showPairFloatPerc(0, ln, "Out chunks total", oct, ocd, "/s")
	showPairFloatPerc(40, ln, "In chunks total", ict, ocd, "/s")
	ln += 2
	obd := statistic.OutBytesData.Get()
	ibd := statistic.InBytesData.Get()
	showPairFloat(0, ln, "Out bytes data", obd, "/s")
	showPairFloatPerc(40, ln, "In bytes data", ibd, obd, "/s")
	ln++
	obt := statistic.OutBytesTotal.Get()
	ibt := statistic.InBytesTotal.Get()
	showPairFloatPerc(0, ln, "Out bytes total", obt, obd, "/s")
	showPairFloatPerc(40, ln, "In bytes total", ibt, obd, "/s")
	ln += 2
	showPairFloat(40, ln, "In packets queue", statistic.InPacketsQueue.Get(), "")
	ln++
	showPairFloat(0, ln, "Out chunks queue", statistic.OutChunksQueue.Get(), "")
	showPairFloat(40, ln, "In chunks queue", statistic.InChunksQueue.Get(), "")
	// ln++
	// showPairInt(0, ln, "Count packets", statistic.DebugPackets, "")
	// ln++
	// showPairInt(0, ln, "Count chunks", statistic.DebugChunks, "")
	// ln++
	// showPairInt(0, ln, "Count bytes", statistic.DebugBytes, "")
	// ln++
	// showPairInt(0, ln, "Chunks lost", statistic.ChunksLost, "")
	ln += 2
	for _, log := range statistic.Logs {
		showText(0, ln, log, true)
		ln++
	}
	if isFinalize {
		ln++
		showText(0, ln, "Press any key to exit", true)
	}
	if Screen != nil {
		Screen.Show()
	}
}

func showPairInt(x, y int, name string, val int64, sfx string) {
	tval := fmt.Sprint(val) + sfx
	showPair(x+20, y, name, tval+"   ")
}

func showPairFloat(x, y int, name string, val float64, sfx string) {
	tval := valToString(val) + sfx
	showPair(x+20, y, name, tval+"   ")
}

func showPairFloatPerc(x, y int, name string, val float64, aval float64, sfx string) {
	tval := valToString(val) + sfx
	showPair(x+20, y, name, tval+"   ")
	if aval > 0 {
		tval += fmt.Sprintf(" (%.1f%%)", val*100/aval)
	}
	showPair(x+20, y, name, tval+"   ")
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
	} else if val >= 0.1 {
		return fmt.Sprintf("%.3f", val)
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
			fmt.Print("\n\r")
		}
	}
}
