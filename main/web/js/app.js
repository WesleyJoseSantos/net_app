import { WifiConn, WifiStaConfig, MqttClientConfig, NetSettigs as NetSettings } from "./dtos.js"
import { HttpClient } from "./http_client.js"

var $ = (id) => document.getElementById(id)

var client = new HttpClient()

var net = new NetSettings()

var route = {
    wifi: '/v1/net/wifi',
    mqtt: '/v1/net/mqtt',
    net: '/v1/net',
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
    var loadedNet = loadData("net")

    if(loadedNet) net = loadedNet
    if(btWifi) btWifi.onclick = () => document.location = '/wifi.html'
    if(btMqtt) btMqtt.onclick = () => document.location = '/mqtt.html'
    if(btSave) btSave.onclick = () => client.request('POST', route.net, net)
    if(btResetDev) btResetDev.onclick = () => client.request('GET', route.reset)
    if(btWifiCn) btWifiCn.onclick = btWifiCn_onclick
    if(btMqttCn) btMqttCn.onclick = btMqttCn_onclick
}

async function btWifiCn_onclick() {
    var result = new WifiConn()

    net.wifi.ssid = $('ssid').value
    net.wifi.password = $('password').value
    
    $('btWifiCn').onclick = null
    $('btWifiCn').textContent = 'connecting...'

    result = await client.request('POST', route.wifi, net.wifi)
    
    if (result.status) {
        alert(`Connection successfully established!\nSSID: ${result.ssid}\nIP: ${result.ip}`)
        saveData("net", net)
        document.location = '/mqtt.html'
    } else {
        alert(`Connection failed!`)
        $('btWifiCn').onclick = btWifiCn_onclick
        $('btWifiCn').textContent = 'connect'
    }
}

async function btMqttCn_onclick() {
    net.mqtt.host = $('host').value
    net.mqtt.password = $('password').value
    net.mqtt.port = parseInt($('port').value)
    net.mqtt.transport = parseInt($('transport').value)
    net.mqtt.username = $('username').value

    $('btMqttCn').onclick = null
    $('btMqttCn').textContent = 'connecting...'

    var result = await client.request('POST', route.mqtt, net.mqtt)

    if (result.connected) {
        alert('Connection successfully established!')
        saveData("net", net)
        document.location = '/index.html'
    } else {
        alert('Connection failed!')
        $('btMqttCn').onclick = buttonCn_onclick
        $('btMqttCn').textContent = 'connect'
    }
}

function saveData(key, data)
{
    try {        
        var json = JSON.stringify(data)
        localStorage.setItem(key, json)
    } catch (error) {
        console.error(error)
    }
}

function loadData(key)
{
    try {
        var json = localStorage.getItem(key)
        return JSON.parse(json)
    } catch (error) {
        return null
    }
}