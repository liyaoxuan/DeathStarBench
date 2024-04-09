package main

import (
	"log"

	"github.com/opentracing/opentracing-go"
	"github.com/spf13/viper"
	"github.com/uber/jaeger-client-go"
)


func SetUpTracer(config_file_path string, service string) {
	viper.SetConfigFile(config_file_path)
	if err := viper.ReadInConfig(); err != nil {
		log.Fatalf("Failed to read config file: %v", err)
	}
	sender, _ := jaeger.NewUDPTransport(viper.Sub("reporter").GetString("localAgentHostPort"), 0)
	tracer, _ := jaeger.NewTracer(service, jaeger.NewConstSampler(true), jaeger.NewRemoteReporter(sender))
	opentracing.InitGlobalTracer(tracer)
}