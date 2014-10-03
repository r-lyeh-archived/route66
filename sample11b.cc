#include <iostream>
#include <fstream>
#include "route66.hpp"

extern std::string html_template;

int main()
{
    // add a web server, with stats in ajax
    route66::create( 8080, "GET /stats",
        []( route66::request &req, std::ostream &headers, std::ostream &content ) {
            int CPUloading = rand() % 100;
            int CPUcore = CPUloading - 30 < 0 ? 0 : rand() % (CPUloading - 30);
            double Disk = rand() % 50 + 50;

            headers << route66::mime(".json");
            content << "{\"cpu\":" << CPUloading << ", \"core\":" << CPUcore << ", \"disk\":" << Disk << "}" << std::endl;
            return 200;
        } );

    route66::create( 8080, "GET /",
        []( route66::request &req, std::ostream &headers, std::ostream &content ) {
            headers << route66::mime(".html");
            content << html_template << "url: " << req.url << std::endl;
            return 200;
        } );

    // do whatever in your app. notice that you could change any route behavior in runtime.
    std::cout << "server ready at localhost:8080 (/)" << std::endl;
    for( char ch ; std::cin >> ch ; )
    {}

    return 0;
}

#define $quote(...) #__VA_ARGS__
std::string html_template = $quote(
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
    <title>sample #2</title>

<script type="text/javascript" src="http://www.jqueryflottutorial.com/js/lib/jquery-1.8.3.min.js"></script>
<!--[if lte IE 8]><script language="javascript" type="text/javascript" src="http://www.jqueryflottutorial.com/js/flot/excanvas.min.js"></script><![endif]-->
<script type="text/javascript" src="http://www.jqueryflottutorial.com/js/flot/jquery.flot.min.js"></script>
<script type="text/javascript" src="http://www.jqueryflottutorial.com/js/flot/jquery.flot.time.js"></script>
<script type="text/javascript" src="http://www.jqueryflottutorial.com/js/flot/jshashtable-2.1.js"></script>
<script type="text/javascript" src="http://www.jqueryflottutorial.com/js/flot/jquery.numberformatter-1.2.3.min.js"></script>
<script type="text/javascript" src="http://www.jqueryflottutorial.com/js/flot/jquery.flot.symbol.js"></script>
<script type="text/javascript" src="http://www.jqueryflottutorial.com/js/flot/jquery.flot.axislabels.js"></script>
<script>
var cpu = [], cpuCore = [], disk = [];
var dataset;
var totalPoints = 100;
var updateInterval = 500;
var now = new Date().getTime();

var options = {
    series: {
        lines: {
            lineWidth: 1.2
        },
        bars: {
            align: "center",
            fillColor: { colors: [{ opacity: 1 }, { opacity: 1}] },
            barWidth: 500,
            lineWidth: 1
        }
    },
    xaxis: {
        mode: "time",
        tickSize: [60, "second"],
        tickFormatter: function (v, axis) {
            var date = new Date(v);

            if (date.getSeconds() % 20 == 0) {
                var hours = date.getHours() < 10 ? "0" + date.getHours() : date.getHours();
                var minutes = date.getMinutes() < 10 ? "0" + date.getMinutes() : date.getMinutes();
                var seconds = date.getSeconds() < 10 ? "0" + date.getSeconds() : date.getSeconds();

                return hours + ":" + minutes + ":" + seconds;
            } else {
                return "";
            }
        },
        axisLabel: "Time",
        axisLabelUseCanvas: true,
        axisLabelFontSizePixels: 12,
        axisLabelFontFamily: 'Verdana, Arial',
        axisLabelPadding: 10
    },
    yaxes: [
        {
            min: 0,
            max: 100,
            tickSize: 5,
            tickFormatter: function (v, axis) {
                if (v % 10 == 0) {
                    return v + "%";
                } else {
                    return "";
                }
            },
            axisLabel: "CPU loading",
            axisLabelUseCanvas: true,
            axisLabelFontSizePixels: 12,
            axisLabelFontFamily: 'Verdana, Arial',
            axisLabelPadding: 6
        }, {
            max: 5120,
            position: "right",
            axisLabel: "Disk",
            axisLabelUseCanvas: true,
            axisLabelFontSizePixels: 12,
            axisLabelFontFamily: 'Verdana, Arial',
            axisLabelPadding: 6
        }
    ],
    legend: {
        noColumns: 0,
        position:"nw"
    },
    grid: {
        backgroundColor: { colors: ["#ffffff", "#EDF5FF"] }
    }
};

function initData() {
    for (var i = 0; i < totalPoints; i++) {
        var temp = [now += updateInterval, 0];

        cpu.push(temp);
        cpuCore.push(temp);
        disk.push(temp);
    }
};

function GetData() {
    $.ajaxSetup({ cache: false });

    $.ajax({
        url: "stats",
        type: "GET",
        dataType: 'json',
        success: update,
        error: function () {
            setTimeout(GetData, updateInterval);
        }
        /*  type: "POST",
            data: content,
            complete: function(data) {
                $('#shop section table tbody').append(data);
            },
        */
    });
};

var temp;

function update(_data) {
    cpu.shift();
    cpuCore.shift();
    disk.shift();

    now += updateInterval;

    temp = [now, _data.cpu];
    cpu.push(temp);

    temp = [now, _data.core];
    cpuCore.push(temp);

    temp = [now, _data.disk];
    disk.push(temp);

    dataset = [
        { label: "CPU:" + _data.cpu + "%", data: cpu, lines: { fill: true, lineWidth: 1.2 }, color: "#00FF00" },
        { label: "Disk:" + _data.disk + "KB", data: disk, color: "#0044FF", bars: { show: true }, yaxis: 2 },
        { label: "CPU Core:" + _data.core + "%", data: cpuCore, lines: { lineWidth: 1.2}, color: "#FF0000" }
    ];

    $.plot($("#flot-placeholder1"), dataset, options);
    setTimeout(GetData, updateInterval);
}


$(document).ready(function () {
    initData();

    dataset = [
        { label: "CPU", data: cpu, lines:{fill:true, lineWidth:1.2}, color: "#00FF00" },
        { label: "Disk:", data: disk, color: "#0044FF", bars: { show: true }, yaxis: 2 },
        { label: "CPU Core", data: cpuCore, lines: { lineWidth: 1.2}, color: "#FF0000" }
    ];

    $.plot($("#flot-placeholder1"), dataset, options);
    setTimeout(GetData, updateInterval);
});



</script>
<!-- HTML -->
<div id="flot-placeholder1" style="width:550px;height:300px;margin:0 auto"></div>
);

