let graphManualChart;
let measurements = {};
let curMeasureIndex = 0;

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

function showMeasurement(name, index) {
    if (measurements[index] == undefined) {
        return;
    }

    if (!graphManualChart.data.datasets.every(d => d.mIndex != index)) {
        return;
    }

    let points = measurements[index].measure;

    let dataset = {
        label: name,
        data: points,
        borderColor: "red",
        fill: false,
        mIndex: index
    }
    graphManualChart.data.datasets.push(dataset);
    graphManualChart.update();
}

function hideMeasurement(index) {
    if (index === undefined) {
        return;
    }

    let dIdx = graphManualChart.data.datasets.findIndex(x => x.mIndex === index);
    if (dIdx != -1) {
        graphManualChart.data.datasets.splice(dIdx, 1);
        graphManualChart.update();
    }
}

function tryShowMeasurement() {
    let select = document.getElementById("measurements");
    let index = select.value;
    showMeasurement(index, index);
}

function tryHideMeasurement() {
    let select = document.getElementById("measurements");
    let index = select.value;
    hideMeasurement(index);
}

function saveMeasurement(index, measure) {
    measurements[index] = {
        measure: measure,
        date: Date.now()
    };
}

function addMeasureToList(index) {
    let select = document.getElementById("measurements");
    let opt = document.createElement("option");
    opt.value = index;
    opt.innerHTML = `Измерение ${index}`;

    select.appendChild(opt);
}

function getRandomData(amount = 3) {
    return [...Array(amount).keys()]
        .map(i => ({
            ampere: Math.random() * 3,
            volt: i
        }));
}

async function measure() {
    let input = document.getElementById("inputChannel");
    let raise = document.getElementById("raiseChannel");

    let data = await fetch(`/metering?in=${input.value}&raise=${raise.value}&from=-3&to=3&step=0.1`)
        .then(body => body.json())
        .catch(e => console.log(`Error: ${e}`));

    curMeasureIndex++;
    saveMeasurement(curMeasureIndex, data.map(p => ({ x: p.volt, y: p.ampere })));
    addMeasureToList(curMeasureIndex);
}

initManualGraph();