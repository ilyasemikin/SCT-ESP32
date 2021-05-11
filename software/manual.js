let graphManualChart;
let measurements = {};

function initManualGraph() {
    let options = {
        scales: {
            x: {
                type: 'linear',
            }
        }
    };

    let config = {
        type: 'line',
        options: options
    };

    graphManualChart = new Chart(
        document.getElementById('manualGraph'),
        config
    );
}

function showMeasurementOnGraph(name, points) {
    points.forEach(p => console.log(`${p.x}`));
    let dataset = {
        label: name,
        data: points
    }
    graphManualChart.data.datasets.push(dataset);
    graphManualChart.update();
}

function addMeasurement(name) {
    let item = document.getElementById("measurements");
    let opt = document.createElement("option");
    opt.value = name;
    opt.innerHTML = name;
    item.appendChild(opt);
}

async function measure() {
    let input = document.getElementById("inputChannel");
    let raise = document.getElementById("raiseChannel");

    let data = await fetch(`http://192.168.0.140/metering?in=${input.value}&raise=${raise.value}&from=-3&to=3&step=0.1`)
        .then(body => body.json())
        .catch(e => console.log(`Error: ${e}`));

    let name = new Date().toString();

    measurements[name] = data;
    addMeasurement(name);
    showMeasurementOnGraph(name, data.map(p => ({ x: p.volt, y: p.ampere })));
}

initManualGraph();