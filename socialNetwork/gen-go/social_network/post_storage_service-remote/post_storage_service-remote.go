// Autogenerated by Thrift Compiler (0.12.0)
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING

package main

import (
        "context"
        "flag"
        "fmt"
        "math"
        "net"
        "net/url"
        "os"
        "strconv"
        "strings"
        "github.com/apache/thrift/lib/go/thrift"
        "social_network"
)


func Usage() {
  fmt.Fprintln(os.Stderr, "Usage of ", os.Args[0], " [-h host:port] [-u url] [-f[ramed]] function [arg1 [arg2...]]:")
  flag.PrintDefaults()
  fmt.Fprintln(os.Stderr, "\nFunctions:")
  fmt.Fprintln(os.Stderr, "  void StorePost(i64 req_id, Post post,  carrier)")
  fmt.Fprintln(os.Stderr, "  Post ReadPost(i64 req_id, i64 post_id,  carrier)")
  fmt.Fprintln(os.Stderr, "   ReadPosts(i64 req_id,  post_ids,  carrier)")
  fmt.Fprintln(os.Stderr)
  os.Exit(0)
}

type httpHeaders map[string]string

func (h httpHeaders) String() string {
  var m map[string]string = h
  return fmt.Sprintf("%s", m)
}

func (h httpHeaders) Set(value string) error {
  parts := strings.Split(value, ": ")
  if len(parts) != 2 {
    return fmt.Errorf("header should be of format 'Key: Value'")
  }
  h[parts[0]] = parts[1]
  return nil
}

func main() {
  flag.Usage = Usage
  var host string
  var port int
  var protocol string
  var urlString string
  var framed bool
  var useHttp bool
  headers := make(httpHeaders)
  var parsedUrl *url.URL
  var trans thrift.TTransport
  _ = strconv.Atoi
  _ = math.Abs
  flag.Usage = Usage
  flag.StringVar(&host, "h", "localhost", "Specify host and port")
  flag.IntVar(&port, "p", 9090, "Specify port")
  flag.StringVar(&protocol, "P", "binary", "Specify the protocol (binary, compact, simplejson, json)")
  flag.StringVar(&urlString, "u", "", "Specify the url")
  flag.BoolVar(&framed, "framed", false, "Use framed transport")
  flag.BoolVar(&useHttp, "http", false, "Use http")
  flag.Var(headers, "H", "Headers to set on the http(s) request (e.g. -H \"Key: Value\")")
  flag.Parse()
  
  if len(urlString) > 0 {
    var err error
    parsedUrl, err = url.Parse(urlString)
    if err != nil {
      fmt.Fprintln(os.Stderr, "Error parsing URL: ", err)
      flag.Usage()
    }
    host = parsedUrl.Host
    useHttp = len(parsedUrl.Scheme) <= 0 || parsedUrl.Scheme == "http" || parsedUrl.Scheme == "https"
  } else if useHttp {
    _, err := url.Parse(fmt.Sprint("http://", host, ":", port))
    if err != nil {
      fmt.Fprintln(os.Stderr, "Error parsing URL: ", err)
      flag.Usage()
    }
  }
  
  cmd := flag.Arg(0)
  var err error
  if useHttp {
    trans, err = thrift.NewTHttpClient(parsedUrl.String())
    if len(headers) > 0 {
      httptrans := trans.(*thrift.THttpClient)
      for key, value := range headers {
        httptrans.SetHeader(key, value)
      }
    }
  } else {
    portStr := fmt.Sprint(port)
    if strings.Contains(host, ":") {
           host, portStr, err = net.SplitHostPort(host)
           if err != nil {
                   fmt.Fprintln(os.Stderr, "error with host:", err)
                   os.Exit(1)
           }
    }
    trans, err = thrift.NewTSocket(net.JoinHostPort(host, portStr))
    if err != nil {
      fmt.Fprintln(os.Stderr, "error resolving address:", err)
      os.Exit(1)
    }
    if framed {
      trans = thrift.NewTFramedTransport(trans)
    }
  }
  if err != nil {
    fmt.Fprintln(os.Stderr, "Error creating transport", err)
    os.Exit(1)
  }
  defer trans.Close()
  var protocolFactory thrift.TProtocolFactory
  switch protocol {
  case "compact":
    protocolFactory = thrift.NewTCompactProtocolFactory()
    break
  case "simplejson":
    protocolFactory = thrift.NewTSimpleJSONProtocolFactory()
    break
  case "json":
    protocolFactory = thrift.NewTJSONProtocolFactory()
    break
  case "binary", "":
    protocolFactory = thrift.NewTBinaryProtocolFactoryDefault()
    break
  default:
    fmt.Fprintln(os.Stderr, "Invalid protocol specified: ", protocol)
    Usage()
    os.Exit(1)
  }
  iprot := protocolFactory.GetProtocol(trans)
  oprot := protocolFactory.GetProtocol(trans)
  client := social_network.NewPostStorageServiceClient(thrift.NewTStandardClient(iprot, oprot))
  if err := trans.Open(); err != nil {
    fmt.Fprintln(os.Stderr, "Error opening socket to ", host, ":", port, " ", err)
    os.Exit(1)
  }
  
  switch cmd {
  case "StorePost":
    if flag.NArg() - 1 != 3 {
      fmt.Fprintln(os.Stderr, "StorePost requires 3 args")
      flag.Usage()
    }
    argvalue0, err161 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err161 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    arg162 := flag.Arg(2)
    mbTrans163 := thrift.NewTMemoryBufferLen(len(arg162))
    defer mbTrans163.Close()
    _, err164 := mbTrans163.WriteString(arg162)
    if err164 != nil {
      Usage()
      return
    }
    factory165 := thrift.NewTJSONProtocolFactory()
    jsProt166 := factory165.GetProtocol(mbTrans163)
    argvalue1 := social_network.NewPost()
    err167 := argvalue1.Read(jsProt166)
    if err167 != nil {
      Usage()
      return
    }
    value1 := argvalue1
    arg168 := flag.Arg(3)
    mbTrans169 := thrift.NewTMemoryBufferLen(len(arg168))
    defer mbTrans169.Close()
    _, err170 := mbTrans169.WriteString(arg168)
    if err170 != nil { 
      Usage()
      return
    }
    factory171 := thrift.NewTJSONProtocolFactory()
    jsProt172 := factory171.GetProtocol(mbTrans169)
    containerStruct2 := social_network.NewPostStorageServiceStorePostArgs()
    err173 := containerStruct2.ReadField3(jsProt172)
    if err173 != nil {
      Usage()
      return
    }
    argvalue2 := containerStruct2.Carrier
    value2 := argvalue2
    fmt.Print(client.StorePost(context.Background(), value0, value1, value2))
    fmt.Print("\n")
    break
  case "ReadPost":
    if flag.NArg() - 1 != 3 {
      fmt.Fprintln(os.Stderr, "ReadPost requires 3 args")
      flag.Usage()
    }
    argvalue0, err174 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err174 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    argvalue1, err175 := (strconv.ParseInt(flag.Arg(2), 10, 64))
    if err175 != nil {
      Usage()
      return
    }
    value1 := argvalue1
    arg176 := flag.Arg(3)
    mbTrans177 := thrift.NewTMemoryBufferLen(len(arg176))
    defer mbTrans177.Close()
    _, err178 := mbTrans177.WriteString(arg176)
    if err178 != nil { 
      Usage()
      return
    }
    factory179 := thrift.NewTJSONProtocolFactory()
    jsProt180 := factory179.GetProtocol(mbTrans177)
    containerStruct2 := social_network.NewPostStorageServiceReadPostArgs()
    err181 := containerStruct2.ReadField3(jsProt180)
    if err181 != nil {
      Usage()
      return
    }
    argvalue2 := containerStruct2.Carrier
    value2 := argvalue2
    fmt.Print(client.ReadPost(context.Background(), value0, value1, value2))
    fmt.Print("\n")
    break
  case "ReadPosts":
    if flag.NArg() - 1 != 3 {
      fmt.Fprintln(os.Stderr, "ReadPosts requires 3 args")
      flag.Usage()
    }
    argvalue0, err182 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err182 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    arg183 := flag.Arg(2)
    mbTrans184 := thrift.NewTMemoryBufferLen(len(arg183))
    defer mbTrans184.Close()
    _, err185 := mbTrans184.WriteString(arg183)
    if err185 != nil { 
      Usage()
      return
    }
    factory186 := thrift.NewTJSONProtocolFactory()
    jsProt187 := factory186.GetProtocol(mbTrans184)
    containerStruct1 := social_network.NewPostStorageServiceReadPostsArgs()
    err188 := containerStruct1.ReadField2(jsProt187)
    if err188 != nil {
      Usage()
      return
    }
    argvalue1 := containerStruct1.PostIds
    value1 := argvalue1
    arg189 := flag.Arg(3)
    mbTrans190 := thrift.NewTMemoryBufferLen(len(arg189))
    defer mbTrans190.Close()
    _, err191 := mbTrans190.WriteString(arg189)
    if err191 != nil { 
      Usage()
      return
    }
    factory192 := thrift.NewTJSONProtocolFactory()
    jsProt193 := factory192.GetProtocol(mbTrans190)
    containerStruct2 := social_network.NewPostStorageServiceReadPostsArgs()
    err194 := containerStruct2.ReadField3(jsProt193)
    if err194 != nil {
      Usage()
      return
    }
    argvalue2 := containerStruct2.Carrier
    value2 := argvalue2
    fmt.Print(client.ReadPosts(context.Background(), value0, value1, value2))
    fmt.Print("\n")
    break
  case "":
    Usage()
    break
  default:
    fmt.Fprintln(os.Stderr, "Invalid function ", cmd)
  }
}
