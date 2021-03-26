let canvas = document.getElementById("graph");
let ctx = canvas.getContext("2d");

let xUnitIntervalPx = 110.0;
let yUnitIntervalPx = 10000000.0;

let graphOffsetPx = 20;

function getRelativePoint(point, w, h) {
    return { x: point.x * w, y: point.y * h };
}

function getAbsolutePoint(point) {
    return {
        x: graphOffsetPx + point.x,
        y: canvas.height - graphOffsetPx - point.y
    };
}

function getPoint(point) {
    return getAbsolutePoint(getRelativePoint(point, xUnitIntervalPx, yUnitIntervalPx));
}

function drawGraph(points, color) {
    if (points.length == 0) {
        return;
    }

    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.lineWidth = 2.0;

    ctx.beginPath();

    let prev = getPoint(points[0]);
    for (let i = 1; i < points.length; i++) {
        let point = getPoint(points[i]);

        console.log("from;", prev.x, prev.y, "to: ", point.x, point.y);

        ctx.fillRect(prev.x - 2, prev.y - 2, 4, 4);
        ctx.moveTo(prev.x, prev.y);
        ctx.lineTo(point.x, point.y);

        prev = point;
    }
    ctx.fillRect(prev.x - 2, prev.y - 2, 4, 4);

    ctx.stroke();
}

function drawAxisXLabels(axisName, color = "black") {
    let countLabels = (canvas.width - 2 * graphOffsetPx) / xUnitIntervalPx;
    
    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.beginPath();

    let yLine = canvas.height - graphOffsetPx;
    let y = canvas.height - graphOffsetPx / 5;
    for (let i = 0; i < countLabels; i++) { 
        let x = graphOffsetPx + i * xUnitIntervalPx;
        ctx.moveTo(x, yLine);
        ctx.lineTo(x, yLine + 5);

        ctx.fillText(`${i}`, x - 5, y);
    }
    ctx.fillText(axisName, canvas.width - graphOffsetPx / 2, yLine);
    ctx.stroke();
}

function drawAxisYLabels(axisName, color = "black") {
    let countLabels = (canvas.height - 2 * graphOffsetPx) / yUnitIntervalPx;
    
    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.beginPath();

    let xLine = graphOffsetPx;
    let x = graphOffsetPx / 5;
    for (let i = 0; i < countLabels; i++) { 
        let y = canvas.height - (graphOffsetPx + i * yUnitIntervalPx);
        ctx.moveTo(xLine, y);
        ctx.lineTo(xLine - 5, y);

        ctx.fillText(`${i}`, x, y + 5);
    }
    ctx.fillText(axisName, xLine, graphOffsetPx / 2);
    ctx.stroke();
}

function drawAxis(color = "black") {
    ctx.strokeStyle = color;
    ctx.lineWidth = 1.0;

    let x0 = graphOffsetPx;
    let y0 = graphOffsetPx;
    let x1 = canvas.width - graphOffsetPx;
    let y1 = canvas.height - graphOffsetPx;
    
    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.lineTo(x0, y1);
    ctx.lineTo(x1, y1);
    ctx.stroke();
}

function draw(points, xName = "x", yName = "y", color = "red") {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    drawGraph(points, color);
    drawAxis();
    drawAxisXLabels(xName);
    drawAxisYLabels(yName);
}