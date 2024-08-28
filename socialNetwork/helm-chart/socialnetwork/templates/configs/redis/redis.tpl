{{- define "socialnetwork.templates.redis.redis.conf"  }}
io-threads 8
io-threads-do-reads yes
port 6379
tls-port 0

tls-cert-file /keys/server.crt
tls-key-file /keys/server.key

tls-auth-clients no

save ""
{{- end }}
