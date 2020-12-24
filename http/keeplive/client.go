package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
)

var url string = "http://192.168.2.132:8080/time"

func main() {
	//var wg sync.WaitGroup
	//wg.Add(1)
	client := &http.Client{}

	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return
	}
	header := req.Header
	header.Set("Connection", "keep-alive")

	resp, err := client.Do(req)
	if err != nil {
		log.Print(err)
		panic(err)
	}

	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)

	fmt.Println("response: " + string(body))

	//wg.Wait()
}
