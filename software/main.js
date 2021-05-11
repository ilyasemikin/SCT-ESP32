let tabHandler = {
    "graphAuto": null,
    "graphManual": null,
    "settings": loadSettings,
}

function openTab(event, name) {
    var tabs = document.getElementsByClassName("tab");
    for (var i = 0; i < tabs.length; i++) {
        tabs[i].style.display = "none";
    }

    var links = document.getElementsByClassName("tabLink");
    for (var i = 0; i < links.length; i++) {
        links[i].className = links[i].className.replace(" active", "");
    }

    document.getElementById(name).style.display = "block";
    event.currentTarget.className += " active";

    let handler = tabHandler[name];
    if (handler != null) {
        handler();
    }
}

document.getElementById("defaultOpen").click();