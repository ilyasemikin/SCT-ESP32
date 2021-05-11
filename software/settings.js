async function loadSettings() {
    await fetch("http://192.168.0.140/device_info")
        .then(body => body.json()
            .then(json => {
                let ssidInput = document.getElementById("wlan_ssid");
                let ipInput = document.getElementById("wlan_ip");
                ssidInput.value = json.ssid;
                ipInput.value = json.ip;
            }))
        .catch(x => console.log(e => `Error ${e}`));
}

async function reconnect() {
    let ssidInput = document.getElementById("wlan_ssid");
    let passwordInput = document.getElementById("wlan_password");
    
    let ssid = ssidInput.value;
    let password = passwordInput.value;

    if (ssid === "" || password === "") {
        return;
    }

    await fetch(`http://192.168.0.140/reconnect?ssid=${ssid}&${password}`)
        .then()
        .catch(e => console.log(`Error ${e}`));
}