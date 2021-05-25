let graphManualChart;
let measurements = {};
let i = 0;

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

function showMeasurement(name, points) {
    points.forEach(p => console.log(`${p.x}`));
    let dataset = {
        label: name,
        data: points
    }
    graphManualChart.data.datasets.push(dataset);
    graphManualChart.update();
}

async function measure() {
    let input = document.getElementById("inputChannel");
    let raise = document.getElementById("raiseChannel");

    let data = await fetch(`/metering?in=${input.value}&raise=${raise.value}&from=-3&to=3&step=0.1`)
        .then(body => body.json())
        .catch(e => console.log(`Error: ${e}`));

    measurements[`${i}`] = data;
    showMeasurement(`${i}`, data.map(p => ({ x: p.volt, y: p.ampere })));
}

initManualGraph();