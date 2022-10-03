class WifiStaConfig {
    ssid = ''
    password = ''
}

class WifiConn {
    ip = ''
    ssid = ''
    status = ''
}

class MqttTransport {
    static UNKNOWN = 0
    static OVER_TCP = 1
    static OVER_SSL = 2
    static OVER_WS = 3
    static OVER_WSS = 4
}

class MqttClientConfig {
    host = ''
    port = 1883
    username = ''
    password = ''
    transport = MqttTransport.OVER_TCP
}

export { WifiStaConfig, WifiConn, MqttClientConfig }