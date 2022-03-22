window.onload = () => {
    const graph = document.getElementById('temp-graph');
    //updateGraph(graph);
    const interval = setInterval(() => updateGraph(graph), 1000);
    const setTempButton = document.getElementById("temp-btn");
    setTempButton.addEventListener('click', event => {
        const temp = document.getElementById('temp-range').value;
        fetch("/api/v1/set_temp", {
            method: "POST",
            body: temp
        }).then(response => response.text()).then(text => {
            console.log("Set value!");
        });
    });
    const tempRange = document.getElementById('temp-range');
    fetch("/api/v1/get_temp").then(response => response.text()).then(text => {
        tempRange.value = text;
    });
    const updateGraph = () => {
        const maxTemp = 120;
        graphWidth = graph.viewBox.animVal.width;
        // TODO: offset graph and add degrees
        graphHeight = graph.viewBox.animVal.height - 15;
        console.log();
        fetch("/api/v1/get_timestep").then(response => response.text()).then(timestep => {
            fetch("/api/v1/get_data").then(response => response.text()).then(text => {
                console.log(timestep);
                const array = text.split(" ");
                const hlines = 11;
                const vlines = Math.max(9, (array.length-2));
                const data = array.map((entry, i) => ({
                    time: (graphWidth * i / (array.length-1)),
                    temp: (graphHeight * (1 - entry / maxTemp))
                }));
                console.log(data);
                graph.innerHTML = `
                ${[...Array(hlines).keys()].map(
                    h => `<line x1="0" y1="${(h+1)*graphHeight/(hlines+1)}"
                           x2="${graphWidth}"
                           y2="${(h+1)*graphHeight/(hlines+1)}"/>`).join("\n")}
                ${[...Array(vlines).keys()].map(
                    v => {
                        const x = (v+1)*graphWidth/(vlines+1);
                        const bufferTime = array.length * timestep;
                        return `<line x1="${x}" y1="0" 
                        x2="${x}"
                        y2="${graphHeight}"/>
                        <text x="${x}" y="${graphHeight + 14}">T-${Math.round(bufferTime / (v+1) / 10) / 100}</text>`
                    }).join("\n")}
                <polyline
                   fill="none"
                   stroke-width="1"
                   points="${data.map(element => `${element.time},${element.temp}`).join(",")}"/>`;
            });
        });
    }
}