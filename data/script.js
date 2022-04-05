const API_PATH = "/api/v1";
var running = false;
async function getData(endpoint) {
    const response = await fetch(API_PATH.trim('/') + '/' + endpoint);
    return await response.text();
}

async function setData(endpoint, data) {
    const response = await fetch(API_PATH.trim('/') + '/' + endpoint,
    {
        method: 'POST',
        body: data
    });
    return await response.text();
}

window.onload = async () => {
    const graph = document.getElementById('temp-graph');

    // Send POST request to the server with the temperature double as payload
    const setTempButton = document.getElementById('temp-btn');
    setTempButton.addEventListener('click', async event => {
        const temp = document.getElementById('temp-range').getElementsByClassName('display')[0].value;
        await setData('set_temp', temp);
    });

    // Send POST request to the server with timer information as milliseconds
    const setTimeButton = document.getElementById('time-btn');
    setTimeButton.addEventListener('click', async event => {
        // Convert time to millis
        const hours = document.getElementById('hours-range')
                      .getElementsByClassName('display')[0];
        const minutes = document.getElementById('minutes-range')
                        .getElementsByClassName('display')[0];
        const millis = (hours.value * 60 + minutes.value) * 60000;
        await setData('set_time', millis);
    });

    const fixPadding = display => {
        if (display.value.length > 2) 
            display.value = display.value.substring(1, 3);
        else if (display.value === "")
            display.value = '00';
        else if (display.value  < 10 && display.value.length < 2) 
            display.value = '0' + display.value;
    };


    // Get the current set temperature and update the temp input to reflect
    const tempRange = document.getElementById('temp-range')
        .getElementsByClassName('display')[0];
    const gotTemp = await getData('get_temp');
    tempRange.value = Math.round(gotTemp);
    fixPadding(tempRange);

    // Functionality for the number inputs, with zero padding and to be able to
    // change the input with scroll, click and change updates.
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

        // Adds padding to some number inputs
        if (Array.from(element.classList).includes('padded')) {
            display.addEventListener('input', event => {
                fixPadding(display);
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

    const startBtn = document.getElementById('start-btn');
    const state = await getData('get_state');
    if (state === 'RUNNING') {
        startBtn.innerHTML = 'Stop';
        startBtn.setAttribute('state', 'running');
    }
    else
    {
        startBtn.innerHTML = 'Start';
        startBtn.setAttribute('state', 'stopped');
    }
    startBtn.addEventListener('click', async event => {
        const state = await getData('get_state');
        if (state === 'RUNNING') {
            const response = await getData('stop');
            if (response === 'OK') {
                startBtn.innerHTML = 'Start';
                startBtn.setAttribute('state', 'stopped');
            }
            else {
                console.log("Error: ", response);
            }
        }
        else
        {
            const response = await getData('start');
            if (response === 'OK') {
                startBtn.innerHTML = 'Stop';
                startBtn.setAttribute('state', 'running');
            }
            else {
                console.log("Error: ", response);
            }
        }
    });

    // Gets data from the API and draws the graph
    const updateGraph = async () => {
        const maxTemp = 120;
        const tempResolution = 10;
        const xOffset = 27;
        const yOffset = 0;
        const timeTextOffset = 12;
        graphWidth = graph.viewBox.animVal.width - xOffset;
        graphHeight = graph.viewBox.animVal.height - timeTextOffset;
        const timestep = await getData('get_timestep');
        const text_data = await getData('get_data');

        running = (await getData('get_state') === "RUNNING");

        if (running) {
            // Get the current set temperature and update the temp input to reflect
            const tempRange = document.getElementById('temp-range')
                .getElementsByClassName('display')[0];
            const gotTemp = await getData('get_temp');
            tempRange.value = Math.round(gotTemp);
            fixPadding(tempRange);
            const timerHoursRange = document.getElementById('hours-range')
                .getElementsByClassName('display')[0];
            const timerMinutesRange = document.getElementById('minutes-range')
                .getElementsByClassName('display')[0];
            const gotElapsedTime = await getData('get_elapsed_time');
            const gotTime = await getData('get_time');
            const timeLeft = gotTime - gotElapsedTime;
            timerHoursRange.setAttribute('value', Math.floor(timeLeft / 3600000));
            timerMinutesRange.setAttribute('value', Math.ceil(timeLeft / 60000));
            fixPadding(timerHoursRange);
            fixPadding(timerMinutesRange);
        }
        const array = text_data.split(" ");
        const hlines = maxTemp / tempResolution - 1;
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
                    ${Math.round((hlines-h) * maxTemp / (hlines+1))}°C
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
    }
    await updateGraph();

    const updateTimer = () => {
        // TODO: Show the timer and how long it's left
    };

    // Make the graph update periodically
    const interval = setInterval(() => {
        if (true) {
            updateTimer();
        }
        updateGraph();
    }, 1000);
}