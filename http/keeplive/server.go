package main

import (
	"fmt"
	"log"
	"net/http"
	"time"
)

func main() {
	th := timeHandler(time.RFC1123)
	http.Handle("/time", th)

	log.Println("Listening...")

	http.ListenAndServe(":8080", nil)
}

func timeHandler(format string) http.Handler {

	f := func(resp http.ResponseWriter, req *http.Request) {
		tm := time.Now().Format(format)

		header := resp.Header()
		header.Set("Keep-Alive", "timeout=30")
		header.Set("Connection", "keep-alive")

		fmt.Fprintf(resp, "The time is: %s", tm)
	}

	return http.HandlerFunc(f)
}
