let graphManualChart;
let measurements = {};
let curMeasureIndex = 0;
let i = 0;

// let refMeasurements = [
//     {
//         from: 0.28, to: 0.35,
//         name: "Zener diode",
//         desc: "1N4728A"
//     },
//     {
//         from: 0, to: 0.07,
//         name: "Resistor",
//         desc: "None"
//     },
//     {
//         from: 0.89, to: 0.93,
//         name: "Diode",
//         desc: "2Д202Р"
//     },
//     {
//         from: 0.70, to: 0.8,
//         name: "Diode",
//         desc: "2Д103А "
//     }
// ]

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
                    align: 'end',
                    position: 'right',
                    display: true,
                    text: 'mA',
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
                        return value.toPrecision(2);
                    }
                }
            },
            x: {
                //type: 'linear',
                
                display: true,
                title: {
                    align: 'end',
                    display: true,
                    text: 'V',
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

function randomNumber(min, max) { 
    return Math.floor(Math.random() * (max - min) + min);;
} 

function borderColors() {
    let r = randomNumber(0, 255);
    let g = randomNumber(0, 255);
    let b = randomNumber(0, 255);
    let result = 'rgba(' + r + ',' + g + ',' + b + ',' + 0.8 + ')';
    console.log(result);
    return result;
}

function showMeasurement(name, index) {
    if (measurements[index] == undefined) {
        return;
    }

    if (!graphManualChart.data.datasets.every(d => d.mIndex != index)) {
        return;
    }

    let points = measurements[index].measure.map(p => ({ x: p.x, y: p.y * 1000 }));
    i++;
    let dataset = {
        label: `Измерение ${i}`,
        data: points,
        //borderColor: "red",
        borderColor: borderColors(),
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

    let z_more = 0;
    for (let i = 0; i < measure.length; i++) {
        if (Math.abs(measure[i].y) < 0.000003) {
            z_more++;
        }
    }
    //detectItem(z_more / measure.length)

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
            ampere: Math.random() / 1000,
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

// async function detectItem(zeroValue) {
//     let str = "Unknown";
//     let desc = "None"
//     for (let i = 0; i < refMeasurements.length; i++) {
//         if (refMeasurements[i].from <= zeroValue && zeroValue <= refMeasurements[i].to) {
//             str = refMeasurements[i].name;
//             desc = refMeasurements[i].desc;
//             break;
//         }
//     }

//     let p = document.getElementById("calcItemName");
//     p.innerHTML = str;

//     await new Promise(resolve => setTimeout(resolve, ((Math.random() * 5).toFixed() + 3) * 1000));   
//     let pDesc = document.getElementById("calcItemDesc");
//     pDesc.innerHTML = desc;
//     alert("Loaded");
// }

async function calibrate() {
    await fetch("/channel_test?in=1&raise=1&value=0")
        .then(_ => alert("Calibrate done"))
        .catch(_ => console.log(`Error ${e}`));
}

async function measure() {
    let input = document.getElementById("inputChannel");
    let raise = document.getElementById("raiseChannel");

    let data = await fetch(`/metering?in=${input.value}&raise=${raise.value}&from=-3.3&to=3.3&step=0.1`)
        .then(body => body.json())
        .catch(e => console.log(`Error: ${e}`));
    // let data = getRandomData(10);

    let index = saveMeasurement(data.map(p => ({ x: p.volt, y: p.ampere })));
    addMeasureToList(index);
}

initManualGraph();