google.charts.load('current', {'packages':['gauge','corechart']});
// Display clock above air quality gauge
function startClock() {
  var now    = new Date();
  var hour   = now.getHours();
  var minute = now.getMinutes();
  var second = now.getSeconds();
  minute = minute < 10 ? "0" + minute : minute;
  second = second < 10 ? "0" + second : second;
  document.getElementById("clock_div").innerHTML =
  hour + ":" + minute + ":" + second;
  var tmp = setTimeout(startClock, 500);
}
google.charts.setOnLoadCallback(drawChart);

function drawChart() {
  var data = google.visualization.arrayToDataTable([
    ['Label', 'Value'],
    ['CO2', 0]
  ]);
  
  var options = {
    redFrom: 2000, redTo: 5000,
    yellowFrom:1000, yellowTo: 1999,
    greenFrom:0, greenTo: 999,
    max: 5000,
    minorTicks: 10,
    majorTicks: ["0","1000","2000","3000","4000","5000"]
  };
  
  var chart = new google.visualization.Gauge(document.getElementById('chart_div'));
  chart.draw(data, options);


  var data2 = google.visualization.arrayToDataTable([
    ['Time', 'CO2'],
    ['0',  0]]);

  var options2 = {
    title: 'CO2 density',
    hAxis: {textPosition: 'none'},
    vAxis: {title:'PPM', viewWindow: {min:0,max:3000}},
    legend: { position: 'bottom' }  
  };

  var chart2 = new google.visualization.LineChart(document.getElementById('line_chart_div'));
  chart2.draw(data2, options2);
  var n=1;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        var obj = JSON.parse(this.responseText);
        var pms = obj.co2;

        console.log(obj);
        if ( obj.status != "measuring" ) {
            document.getElementById("progress").style.visibility = "visible";
            document.getElementById("progress").setAttribute("value", obj.readystatus);
        } else {
            document.getElementById("progress").style.visibility = "hidden";
        }

        document.getElementById('status_div').innerHTML = "Status: "+obj.status;
        // if ( obj.status == "preheating" ) {
        //     document.getElementById('status_div').innerHTML += " " + obj.readystatus + "%";
        // }
        data.setValue(0, 1, pms);
        chart.draw(data, options);

        // options2 = {
        //   title: 'CO2 density',
        //   hAxis: {textPosition: 'none'},
        //   vAxis: {title:'CO2 density in PPM', viewWindow: {min:0,max:3000}},
        //   legend: { position: 'bottom' }  
        // };
        // data2.addRow([n.toString(), parseInt(pms)]);
        data2.addRow([Date().toString(), parseInt(pms)]);
        chart2.draw(data2, options2);
        n++;
      }
    };
    xhttp.timeout = 2000;
    xhttp.ontimeout = function() {
      document.getElementById('status_div').innerHTML = "Status: disconnected";
    }
    xhttp.open("GET", "/updatesensorreading", true);
    xhttp.send();
  }, 3000 );
}
