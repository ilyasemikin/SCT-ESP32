let chart = null;

function updateGraph(points) {
    let ctx = document.getElementById('graph').getContext('2d');
    
    if (chart != null) {
        chart.destroy();
    }

    chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: points.map(x => x.x),
            datasets: [{
                label: "Chars",
                data: points.map(x => x.y)
            }]
        },
        options: {
            responsive: true
        }
    });
}

function getPoints(json) {
    let raw = JSON.parse(json);
    return raw.map(item => ({ x: item.volt, y: item.ampere }));
}

let input_in = document.getElementById("in");
let input_out = document.getElementById("out");
let input_from = document.getElementById("from");
let input_to = document.getElementById("to");

function getPointsFromSource() {
    let from = parseFloat(input_from.value);
    let to = parseFloat(input_to.value);
    let in_ = parseInt(input_in.value);
    let out = parseInt(input_out.value);

    fetch(`./metering?from=${from}&to=${to}&in=${in_}&out=${out}`)
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

    updateGraph(getPoints(json));
} 

update_btn = document.getElementById("update_btn");
update_btn.onclick = getPointsFromSource;
