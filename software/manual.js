let graphManualChart;
let measurements = {};
let i = 0;

function initManualGraph() {
    let options = {
        scales: {
            xAxes: {
                type: 'linear',
                display: true,
                //position: 'bottom'
            },

            y: {
                //type: 'linear',
                display: true,
                title: {
                    display: true,
                    text: 'Amperes',
                    color: '#911',
                    font: {
                        family: 'Comic Sans MS',
                        size: 20,
                        weight: 'bold',
                        lineHeight: 1.2,
                    },
                },
            },
            x: {
                //type: 'linear',
                display: true,
                title: {
                    display: true,
                    text: 'Volts',
                    color: '#911',
                    font: {
                        family: 'Comic Sans MS',
                        size: 20,
                        weight: 'bold',
                        lineHeight: 1.2,
                    },
                },
            },
            
        },
    };

    let config = {
        type: 'line',
        options: options
    };

    graphManualChart = new Chart(
        document.getElementById('manualGraph').getContext('2d'),
        config
    );
}

function showMeasurement(name, points) {
    points.forEach(p => console.log(`${p.x}, ${p.y}`));
    
    let dataset = {
        label: name,
        data: points,
        borderColor: "red",
        fill: false
    }
    graphManualChart.data.datasets.push(dataset);
    graphManualChart.update();
}

async function measure() {
    let input = document.getElementById("inputChannel");
    let raise = document.getElementById("raiseChannel");

    let data = await fetch(`./metering?in=${input.value}&raise=${raise.value}&from=-3&to=3&step=0.1`)
        .then(body => body.json())
        .catch(e => console.log(`Error: ${e}`));

    measurements[`${i}`] = data;

    i++;//номер измерения

    showMeasurement(`Измерение №${i}`, data.map(p => ({ x: p.volt, y: p.ampere })));
}

initManualGraph();