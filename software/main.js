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

//Мультиязычность
const select = document.querySelector('select')  
const allLang = ['en', 'ru']

select.addEventListener('change', changeURLLanguage);

//перенаправление на URL с указанием языка
function changeURLLanguage() {
    let lang = select.value;
    location.href = window.location.pathname + '#'+ lang;
    location.reload();
}

function changeLanguage() {
    let hash = window.location.hash;
    hash = hash.substr(1);

    console.log(hash);

    if (!allLang.includes(hash)) {
        location.href = window.location.pathname + '#en';
        location.reload();
    }

    select.value = hash;
    document.querySelector('title').innerHTML = langArr['unit'][hash];
    for (let key in langArr) {
        let elem = document.querySelector('.lng-' + key);
        if (elem) {
            elem.innerHTML = langArr[key][hash];
        }
    }
}

changeLanguage();