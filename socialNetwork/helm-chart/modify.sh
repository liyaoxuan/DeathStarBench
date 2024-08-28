#! /bin/bash

services=$(ls socialnetwork/charts | grep -v tgz)

for service in $services; do
	sed -i '2c nodeName: cpu-16' socialnetwork/charts/$service/values.yaml
done


