import { WifiConn, WifiStaConfig, MqttClientConfig } from "./dtos.js"
import { HttpClient } from "./http_client.js"

var $ = (id) => document.getElementById(id)
var client = new HttpClient()

var route = {
    wifi: '/v1/net/wifi',
    mqtt: '/v1/net/mqtt',
    save: '/v1/net/save',
    reset: '/v1/system/reset'
}

window.onload = init

function init() {
    var btWifi = $('btWifi')
    var btMqtt = $('btMqtt')
    var btSave = $('btSave')
    var btResetDev = $('btResetDev')
    var btWifiCn = $('btWifiCn')
    var btMqttCn = $('btMqttCn')

    if(btWifi) btWifi.onclick = () => document.location = '/wifi.html'
    if(btMqtt) btMqtt.onclick = () => document.location = '/mqtt.html'
    if(btSave) btSave.onclick = () => client.request('GET', route.save)
    if(btResetDev) btResetDev.onclick = () => client.request('GET', route.reset)
    if(btWifiCn) btWifiCn.onclick = btWifiCn_onclick
    if(btMqttCn) btMqttCn.onclick = btMqttCn_onclick
}

async function btWifiCn_onclick() {
    var data = new WifiStaConfig()
    var result = new WifiConn()

    data.ssid = $('ssid').value
    data.password = $('password').value
    
    $('btWifiCn').onclick = null
    $('btWifiCn').textContent = 'connecting...'

    result = await client.request('POST', route.wifi, data)
    
    if (result.status) {
        alert(`Connection successfully established!\nSSID: ${result.ssid}\nIP: ${result.ip}`)
        document.location = '/mqtt.html'
    } else {
        alert(`Connection failed!`)
        $('btWifiCn').onclick = btWifiCn_onclick
        $('btWifiCn').textContent = 'connect'
    }
}

async function btMqttCn_onclick() {
    var data = new MqttClientConfig()

    data.host = $('host').value
    data.password = $('password').value
    data.port = $('port').value
    data.transport = $('transport').selectedIndex
    data.username = $('username').value

    $('btMqttCn').onclick = null
    $('btMqttCn').textContent = 'connecting...'

    var result = await client.request('POST', route.mqtt, data)

    if (result.connected) {
        alert('Connection successfully established!')
        document.location = '/index.html'
    } else {
        alert('Connection failed!')
        $('btMqttCn').onclick = buttonCn_onclick
        $('btMqttCn').textContent = 'connect'
    }
}