docker build -t social-network-microservices:go -f Dockerfile-go . \
&& docker tag social-network-microservices:go yxlee/social-network-microservices:go \
&& docker push yxlee/social-network-microservices:go \
&& sudo crictl rmi docker.io/yxlee/social-network-microservices:go \
&& kubectl delete -n socialnetwork pod `kubectl --namespace socialnetwork get po | grep compose-post-service | awk '{print $1}'` \

