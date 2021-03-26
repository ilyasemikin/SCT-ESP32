function updateGraph(points) {
    draw(points, "V", "A");
}

function getPoints(json) {
    let raw = JSON.parse(json);
    return raw.map(item => ({ x: item.volt, y: item.ampere }));
}

function getPointsFromSource() {
    fetch("./metering")
        .then(response => {
            if (response.status !== 200) {
                console.log(`Metering get error: status code: ${response.status}`);
                return;
            }

            return response.text();
        })
        .then(json => {
            updateGraph(getPoints(json));
        })
}

updateGraph([]);

update_btn = document.getElementById("update_btn");
update_btn.onclick = getPointsFromSource;