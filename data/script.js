window.onload = () => {
    const graph = document.getElementById('temp-graph');

    const interval = setInterval(() => updateGraph(graph), 1000);

    const setTempButton = document.getElementById('temp-btn');
    setTempButton.addEventListener('click', event => {
        const temp = document.getElementById('temp-range').value;
        fetch("/api/v1/set_temp", {
            method: "POST",
            body: temp
        }).then(response => response.text()).then(text => {
            console.log(`Set temperature to: ${text}!`);
        });
    });

    const setTimeButton = document.getElementById('time-btn');
    setTimeButton.addEventListener('click', event => {
        // Convert time to millis
        const hours = document.getElementById('hours-range')
                      .getElementsByClassName('display')[0];
        const minutes = document.getElementById('minutes-range')
                        .getElementsByClassName('display')[0];
        const millis = (hours.value * 60 + minutes.value) * 60000;
        fetch("/api/v1/set_time", {
            method: "POST",
            body: millis
        }).then(response => response.text()).then(text => {
            console.log(`Set timer to: ${text}!`);
        });
    });

    const numberInputs = Array.from(
        document.getElementsByClassName('number-input'));
    numberInputs.forEach(element => {
        const dec = element.getElementsByClassName('dec')[0];
        const display = element.getElementsByClassName('display')[0];
        const inc = element.getElementsByClassName('inc')[0];
        const stepUp = () => {
            display.stepUp();
            if (Array.from(element.classList).includes('padded')) {
                if (display.value < 10) display.value = '0' + display.value;
            }
        };

        const stepDown = () => {
            display.stepDown();
            if (Array.from(element.classList).includes('padded')) {
                if (display.value < 10) display.value = '0' + display.value;
            }
        };

        dec.addEventListener('click', event => {
            stepDown();
        });

        inc.addEventListener('click', event => {
            stepUp();
        });

        if (Array.from(element.classList).includes('padded')) {
            display.addEventListener('input', event => {
                if (display.value < 10) display.value = '0' + display.value;
                if (display.value.length > 2) display.value = display.value.substring(1, 3);
            })
        }

        element.addEventListener('mousewheel', event => {
            event.stopPropagation();
            event.preventDefault();
            if (event.deltaY != 0) {
                if (event.deltaY < 0) stepUp();
                if (event.deltaY > 0) stepDown();
            }
        });
    });

    const tempRange = document.getElementById('temp-range');
    fetch("/api/v1/get_temp").then(response => response.text()).then(text => {
        tempRange.value = text;
    });
    const updateGraph = () => {
        const maxTemp = 120;
        const tempResolution = 10;
        const xOffset = 27;
        const yOffset = 0;
        const timeTextOffset = 12;
        graphWidth = graph.viewBox.animVal.width - xOffset;
        graphHeight = graph.viewBox.animVal.height - timeTextOffset;
        fetch("/api/v1/get_timestep")
        .then(response => response.text()).then(timestep => {
            fetch("/api/v1/get_data")
            .then(response => response.text()).then(text => {
                const array = text.split(" ");
                const hlines = maxTemp / tempResolution - 2;
                const vlines = Math.min(9, (array.length-2));
                const data = array.map((entry, i) => ({
                    x: (graphWidth * i / (array.length-1)) + xOffset,
                    y: (graphHeight * (1 - entry / maxTemp)) + yOffset
                }));
                graph.innerHTML = `
                ${[...Array(hlines).keys()].map(
                    h => {
                        const y = (h+1)*graphHeight/(hlines+1);
                        return `<line x1="${xOffset}" y1="${y}"
                        x2="${graphWidth + xOffset}"
                        y2="${y}"/>
                        <text class="temp-text" x="0" y="${y}">
                            ${Math.round((hlines-h) * maxTemp / (hlines+2))}°C
                        </text>`
                    }).join("\n")}
                ${[...Array(vlines).keys()].map(
                    v => {
                        const x = (v+1)*graphWidth/(vlines+1) + xOffset;
                        const bufferTime = array.length * timestep;
                        return `<line x1="${x}" y1="0" 
                        x2="${x}"
                        y2="${graphHeight}"/>
                        <text class="time-text" x="${x}"
                        y="${graphHeight + timeTextOffset}">
                            T-${Math.round((vlines-v) * bufferTime / 
                            ((vlines+2) * 10)) / 100}s
                        </text>`
                    }).join("\n")}
                <polyline
                   fill="none"
                   stroke-width="1"
                   points="${data.map(element => `${element.x},${element.y}`)
                   .join(",")}"/>`;
            });
        });
    }
}