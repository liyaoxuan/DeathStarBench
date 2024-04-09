package main

import (
	"log"
	"time"

	"github.com/apache/thrift/lib/go/thrift"
	"github.com/gogf/gf/container/gpool"
)

type ThriftClient struct {
	initialized bool
	addr        string
	trans       thrift.TTransport
	iprot       thrift.TProtocol
	oprot       thrift.TProtocol
	client      *thrift.TStandardClient
}

func NewThriftClient(addr string, timeout_ms int) (tc *ThriftClient, err error) {
	tc = new(ThriftClient)
	tc.addr = addr
	tc.trans, err = thrift.NewTSocketTimeout(addr, time.Duration(timeout_ms*1e6))
	if err != nil {
		log.Fatalf("Failed to create socket with addr:%s\n", addr)
		return nil, err
	}
	tc.trans = thrift.NewTFramedTransport(tc.trans)
	protocol_factory := thrift.NewTBinaryProtocolFactoryDefault()
	tc.iprot = protocol_factory.GetProtocol(tc.trans)
	tc.oprot = protocol_factory.GetProtocol(tc.trans)
	tc.client = thrift.NewTStandardClient(tc.iprot, tc.oprot)
	return
}

func (tc *ThriftClient) Init(addr string, timeout_ms int) (err error) {
	tc.addr = addr
	tc.trans, err = thrift.NewTSocketTimeout(addr, time.Duration(timeout_ms*1e6))
	if err != nil {
		log.Fatalf("Failed to create socket with addr:%s\n", addr)
		return err
	}
	tc.trans = thrift.NewTFramedTransport(tc.trans)
	protocol_factory := thrift.NewTBinaryProtocolFactoryDefault()
	tc.iprot = protocol_factory.GetProtocol(tc.trans)
	tc.oprot = protocol_factory.GetProtocol(tc.trans)
	tc.client = thrift.NewTStandardClient(tc.iprot, tc.oprot)
	tc.initialized = true
	return
}

func (tc *ThriftClient) Connect() (err error) {
	if !tc.IsConnected() {
		err = tc.trans.Open()
	}
	return
}

func (tc *ThriftClient) Disconnect() (err error) {
	if tc.IsConnected() {
		err = tc.trans.Close()
	}
	return
}

func (tc *ThriftClient) IsConnected() bool {
	return tc.trans.IsOpen()
}

func (tc *ThriftClient) GetClient() (c *thrift.TStandardClient) {
	c = tc.client
	return
}

type ThriftClientPool struct {
	name         string
	addr         string
	timeout_ms   int
	keepalive_ms int
	pool         *gpool.Pool
	//client *ThriftClient
}

func NewThriftClientPool(name string, addr string, timeout_ms int, keepalive_ms int) (tcp *ThriftClientPool, err error) {
	tcp = new(ThriftClientPool)
	tcp.name = name
	tcp.addr = addr
	tcp.timeout_ms = timeout_ms
	tcp.keepalive_ms = keepalive_ms
	tcp.pool = gpool.New(time.Duration(tcp.keepalive_ms*1e6),
		func() (interface{}, error) {
			tc := new(ThriftClient)
			tc.initialized = false
			return tc, nil
		},
		func(i interface{}) {
			i.(*ThriftClient).Disconnect()
		})
	//tcp.client, err = NewThriftClient(addr, timeout_ms)
	//tcp.client.Connect()
	return
}

func (tcp *ThriftClientPool) Get() *ThriftClient {
	client, err := tcp.pool.Get()
	if err != nil {
		log.Fatalf("Clientpool::Error: fail to get client")
		return nil
	}
	if !client.(*ThriftClient).initialized {
		client.(*ThriftClient).Init(tcp.addr, tcp.timeout_ms)
	}
	client.(*ThriftClient).Connect()
	return client.(*ThriftClient)
	//return tcp.client
}

func (tcp *ThriftClientPool) Push(tc *ThriftClient) {
	tcp.pool.Put(tc)
}
