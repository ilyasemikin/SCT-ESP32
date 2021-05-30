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
                    text: 'Current',
                    color: '#911',
                    font: {
                        family: 'Roboto',
                        size: 20,
                        weight: 'bold',
                        lineHeight: 1.2,
                    },
                },
                ticks: {
                    textStrokeWidth: 3,
                    callback: function(value) {
                        return value.toPrecision(6);//задача точности числа
                        //return (parseFloat(value) * 1000000).toPrecision(3);
                    }
                }
            },
            x: {
                //type: 'linear',
                display: true,
                title: {
                    display: true,
                    text: 'Voltage',
                    color: '#911',
                    font: {
                        family: 'Roboto',
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
        label: 'Измерение ' + Number(name + 1),
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

function listSelectedIndex() {
    return document.getElementById("measurements").index;
}

function tryHideMeasurement() {
    let select = document.getElementById("measurements");
    let index = select.value;
    hideMeasurement(index);
}

function tryRemoveMeasurement() {
    let select = document.getElementById("measurements");
    let index = select.value;
    removeMeasurement(index);
}

function saveMeasurement(measure) {
    let index = curMeasureIndex;

    measurements[curMeasureIndex] = {
        measure: measure,
        time: Date.now()
    };

    while (curMeasureIndex in measurements) {
        curMeasureIndex++;
    }

    return index;
}

function deleteMeasurement(index) {
    if (index in measurements) {
        delete measurements[index];
    }
}

function removeMeasurement(index) {
    if (index in measurements) {
        hideMeasurement(index);
        deleteMeasureList(index);
        deleteMeasurement(index);
    }
}

function addMeasureToList(index) {
    let select = document.getElementById("measurements");
    let opt = document.createElement("option");
    opt.value = index;
    opt.id = `listMeasure${index}`
    opt.innerHTML = `Измерение ${index + 1}`;

    select.appendChild(opt);
}

function deleteMeasureList(index) {
    let select = document.getElementById("measurements");
    let opt = document.getElementById(`listMeasure${index}`);
    select.remove(opt.index);
}

function getRandomData(amount = 3) {
    return [...Array(amount).keys()]
        .map(i => ({
            ampere: Math.random() * 3,
            volt: i
        }));
}

function download(content, fileName, contentType) {
    const a = document.createElement("a");
    const file = new Blob([content], { type: contentType });
    a.href = URL.createObjectURL(file);
    a.download = fileName;
    a.click();
}

function isNumber(value) {
    return typeof value === "number" && isFinite(value);
}

function validateMeasure(item) {
    if ("measure" in item && "time" in item) {
        return item.measure.every(item => 
            "x" in item && isNumber(item.x) && 
            "y" in item && isNumber(item.y));
    }

    return false;
}

function exportMeasurements() {
    let json = JSON.stringify(measurements);
    download(json, "measurements.json", "text/plain");
}

async function measure() {
    let input = document.getElementById("inputChannel");
    let raise = document.getElementById("raiseChannel");

    let data = await fetch(`http://192.168.4.1/metering?in=${input.value}&raise=${raise.value}&from=-3&to=3&step=0.1`)
        .then(body => body.json())
        .catch(e => console.log(`Error: ${e}`));
    // let data = getRandomData(10);

    let index = saveMeasurement(data.map(p => ({ x: p.volt, y: p.ampere })));
    addMeasureToList(index);
}

initManualGraph();