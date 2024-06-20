for service in compose-post-service \
home-timeline-service \
media-service \
post-storage-service \
social-graph-service \
text-service \
unique-id-service \
url-shorten-service \
user-mention-service \
user-service \
user-timeline-service; do
  kubectl logs -n socialnetwork `kubectl --namespace socialnetwork get po | grep $service | awk '{print $1}'` | grep -E "Error|error"
done
