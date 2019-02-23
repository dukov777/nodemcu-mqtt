curl --header Content-Type: application/json --request PUT --data '{broker:broker.hivemq.com}' http://192.168.1.1/settings -max-time 10
mosquitto_sub -h broker.hivemq.com -t outTopic
