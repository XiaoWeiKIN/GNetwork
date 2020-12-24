package main

import (
	"log"
	"net"
)

func main() {
	listen, err := net.Listen("tcp", ":8081")
	if err != nil {
		log.Println("listen error: ", err)

		return
	}

	for {
		log.Print("accept...")
		conn, err := listen.Accept()
		if err != nil {
			log.Println("accept error: ", err)
			break
		}

		go Hadnle(conn)
	}
}

func Hadnle(conn net.Conn) {
	defer conn.Close()

	packet := make([]byte, 1204)
	for {
		// block here if socket is not available for reading data.
		log.Print("read...")
		n, err := conn.Read(packet)
		if err != nil {
			log.Println("read socket error: ", err)
			return
		}
		// same as above, block here if socket is not available for writing.
		log.Print("write...")
		_, _ = conn.Write(packet[:n])
	}
}
