package main

import (
	"fmt"
	"log"
	"net"
	"os"
	"strconv"
	"encoding/json"
	"socialnetwork/gen-go/social_network"

	"github.com/apache/thrift/lib/go/thrift"
)

type ServiceConfig struct {
	Keepalive_ms int
	Addr         string
	Timeout_ms   int
	Port         int
	Connections  int
}

type RedisConfig struct {
	Keepalive_ms int
	Addr         string
	Timeout_ms   int
	Port         int
	Connections  int
	UserCluster  int
	UserReplica  int
}

type MemcachedConfig struct {
	Keepalive_ms   int
	Addr           string
	Timeout_ms     int
	Port           int
	Connections    int
	BinaryProtocol int
}

//type Config struct {
//	SocialGraphMongoDB     	ServiceConfig
//	Secret                 	string
//	MediaService			ServiceConfig
//	UrlShortenMemcached		ServiceConfig
//	SocialGraphService		ServiceConfig
//	UserTimelineRedis		RedisConfig
//	SocialGraphRedis		RedisConfig
//	PostStorageService		ServiceConfig
//	ComposePostRedis		RedisConfig
//	UserTimelineMongoDB		ServiceConfig
//	UserMongoDB				ServiceConfig
//	UserMemcached 			MemcachedConfig
//	Ssl						interface {}
//	TextService				ServiceConfig
//	WriteHomeTimelineService
//	UniqueIDService        	ServiceConfig
//
//}

func main() {
	SetUpTracer("config/jaeger-config.yml", "compose-post-service")
	content, err := os.ReadFile("config/service-config.json")
	if err != nil {
		log.Fatal("Error when opening file: ", err)
	}

	var config map[string]interface{}
	err = json.Unmarshal(content, &config)
	if err != nil {
		log.Fatal("Error during Unmarshal(): ", err)
	}

	var addr, port string
	var keepalive_ms int
	var timeout_ms int
	addr = config["text-service"].(map[string]interface{})["addr"].(string)
	port = strconv.Itoa(int(config["text-service"].(map[string]interface{})["port"].(float64)))
	timeout_ms = int(config["text-service"].(map[string]interface{})["timeout_ms"].(float64))
	keepalive_ms = int(config["text-service"].(map[string]interface{})["keepalive_ms"].(float64))
	text_client_pool, err := NewThriftClientPool("text", net.JoinHostPort(addr, port), timeout_ms, keepalive_ms)
	if err != nil {
		log.Fatal("Failed to create text service client pool")
	}

	addr = config["user-service"].(map[string]interface{})["addr"].(string)
	port = strconv.Itoa(int(config["user-service"].(map[string]interface{})["port"].(float64)))
	timeout_ms = int(config["user-service"].(map[string]interface{})["timeout_ms"].(float64))
	keepalive_ms = int(config["user-service"].(map[string]interface{})["keepalive_ms"].(float64))
	user_client_pool, err := NewThriftClientPool("user", net.JoinHostPort(addr, port), timeout_ms, keepalive_ms)
	if err != nil {
		log.Fatal("Failed to create user service client pool")
	}

	addr = config["media-service"].(map[string]interface{})["addr"].(string)
	port = strconv.Itoa(int(config["media-service"].(map[string]interface{})["port"].(float64)))
	timeout_ms = int(config["media-service"].(map[string]interface{})["timeout_ms"].(float64))
	keepalive_ms = int(config["media-service"].(map[string]interface{})["keepalive_ms"].(float64))
	media_client_pool, err := NewThriftClientPool("media", net.JoinHostPort(addr, port), timeout_ms, keepalive_ms)
	if err != nil {
		log.Fatal("Failed to create media service client pool")
	}

	addr = config["post-storage-service"].(map[string]interface{})["addr"].(string)
	port = strconv.Itoa(int(config["post-storage-service"].(map[string]interface{})["port"].(float64)))
	timeout_ms = int(config["post-storage-service"].(map[string]interface{})["timeout_ms"].(float64))
	keepalive_ms = int(config["post-storage-service"].(map[string]interface{})["keepalive_ms"].(float64))
	post_storage_client_pool, err := NewThriftClientPool("post-storage", net.JoinHostPort(addr, port), timeout_ms, keepalive_ms)
	if err != nil {
		log.Fatal("Failed to create post storage service client pool")
	}

	addr = config["user-timeline-service"].(map[string]interface{})["addr"].(string)
	port = strconv.Itoa(int(config["user-timeline-service"].(map[string]interface{})["port"].(float64)))
	timeout_ms = int(config["user-timeline-service"].(map[string]interface{})["timeout_ms"].(float64))
	keepalive_ms = int(config["user-timeline-service"].(map[string]interface{})["keepalive_ms"].(float64))
	user_timeline_client_pool, err := NewThriftClientPool("user-timeline", net.JoinHostPort(addr, port), timeout_ms, keepalive_ms)
	if err != nil {
		log.Fatal("Failed to create user timeline service client pool")
	}

	addr = config["home-timeline-service"].(map[string]interface{})["addr"].(string)
	port = strconv.Itoa(int(config["home-timeline-service"].(map[string]interface{})["port"].(float64)))
	timeout_ms = int(config["home-timeline-service"].(map[string]interface{})["timeout_ms"].(float64))
	keepalive_ms = int(config["home-timeline-service"].(map[string]interface{})["keepalive_ms"].(float64))
	home_timeline_client_pool, err := NewThriftClientPool("home-timeline", net.JoinHostPort(addr, port), timeout_ms, keepalive_ms)
	if err != nil {
		log.Fatal("Failed to create user timeline service client pool")
	}

	addr = config["unique-id-service"].(map[string]interface{})["addr"].(string)
	port = strconv.Itoa(int(config["unique-id-service"].(map[string]interface{})["port"].(float64)))
	timeout_ms = int(config["unique-id-service"].(map[string]interface{})["timeout_ms"].(float64))
	keepalive_ms = int(config["unique-id-service"].(map[string]interface{})["keepalive_ms"].(float64))
	unique_id_client_pool, err := NewThriftClientPool("unique-id", net.JoinHostPort(addr, port), timeout_ms, keepalive_ms)
	if err != nil {
		log.Fatal("Failed to create unique id service client pool")
	}

	addr = "0.0.0.0"
	port = strconv.Itoa(int(config["compose-post-service"].(map[string]interface{})["port"].(float64)))
	server_socket, err := thrift.NewTServerSocket(net.JoinHostPort(addr, port))
	if err != nil {
		log.Fatalf("Failed to create server socket")
	}
	handler := NewComposePostServiceHander(
		text_client_pool,
		user_client_pool,
		media_client_pool,
		unique_id_client_pool,
		post_storage_client_pool,
		user_timeline_client_pool,
		home_timeline_client_pool,
	)
	processor := social_network.NewComposePostServiceProcessor(handler)
	transport_factory := thrift.NewTFramedTransportFactory(thrift.NewTTransportFactory())
	protocol_factory := thrift.NewTBinaryProtocolFactoryDefault()
	server := thrift.NewTSimpleServer4(processor, server_socket, transport_factory, protocol_factory)
	log.Print(fmt.Println("Starting the compose-post server... on ", addr))
	server.Serve()
}