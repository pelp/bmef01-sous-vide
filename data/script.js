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
        console.log();
        fetch("/api/v1/get_timestep").then(response => response.text()).then(timestep => {
            fetch("/api/v1/get_data").then(response => response.text()).then(text => {
                console.log(timestep);
                const array = text.split(" ");
                const data = array.map((entry, i) => ({
                    time: (graph.viewBox.animVal.width * i / array.length),
                    temp: (graph.viewBox.animVal.height * (1 - entry / 125))
                }));
                console.log(data);
                graph.innerHTML = `
                <polyline
                   fill="none"
                   stroke="#0074d9"
                   stroke-width="3"
                   points="${data.map(element => `${element.time},${element.temp}`).join(",")}"/>`;
            });
        });
    }
}