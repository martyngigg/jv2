# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin and T. Youngs

from flask import Flask
from flask import jsonify
from flask import request

from urllib.request import urlopen
from xml.etree.ElementTree import parse
import lxml.etree as ET

from ast import literal_eval

from datetime import datetime

import nexusInteraction

app = Flask(__name__)

# Shutdown flask server


def shutdown_server():
    serverShutdownFunction = request.environ.get('werkzeug.server.shutdown')
    if serverShutdownFunction is None:
        raise RuntimeError('Not running with the Local Server')
    serverShutdownFunction()

# Get nexus file fields


@app.route('/getNexusFields/<instrument>/<cycle>/<runs>')
def getNexusFields(instrument, cycle, runs):
    runFields = nexusInteraction.runFields(instrument, cycle, runs)
    return jsonify(runFields)

# Get all log data from nexus field


@app.route('/getNexusData/<instrument>/<cycle>/<runs>/<fields>')
def getNexusData(instrument, cycle, runs, fields):
    fieldData = nexusInteraction.fieldData(instrument, cycle, runs, fields)
    return jsonify(fieldData)

# Get instrument cycles


@app.route('/getCycles/<instrument>')
def getCycles(instrument):
    url = 'http://data.isis.rl.ac.uk/journals/'
    url += 'ndx'+instrument+'/journal_main.xml'
    try:
        response = urlopen(url)
    except Exception:
        return jsonify({"response": "ERR. url not found"})
    tree = parse(response)
    root = tree.getroot()
    cycles = []
    for data in root:
        cycles.append(data.get('name'))
    return jsonify(cycles)

# Get cycle run data


@app.route('/getJournal/<instrument>/<cycle>')
def getJournal(instrument, cycle):
    url = 'http://data.isis.rl.ac.uk/journals/ndx'+instrument+'/'+cycle
    try:
        response = urlopen(url)
    except Exception:
        return jsonify({"response": "ERR. url not found"})
    tree = parse(response)
    root = tree.getroot()
    fields = []
    for run in root:
        runData = {}
        for data in run:
            dataId = data.tag.replace(
                '{http://definition.nexusformat.org/schema/3.0}', '')
            try:
                dataValue = data.text.strip()
            except Exception:
                dataValue = data.text
            runData[dataId] = dataValue
        fields.append(runData)
    return jsonify(fields)

# Search all cycles


@app.route('/getAllJournals/<instrument>/<search>')
def getAllJournals(instrument, search):
    allFields = []
    nameSpace = {'data': 'http://definition.nexusformat.org/schema/3.0'}
    cycles = literal_eval(getCycles(instrument).get_data().decode())
    cycles.pop(0)

    startTime = datetime.now()

    for cycle in (cycles):
        print(instrument, " ", cycle)
        url = 'http://data.isis.rl.ac.uk/journals/ndx' + \
            instrument+'/'+str(cycle)
        try:
            response = urlopen(url)
        except Exception:
            return jsonify({"response": "ERR. url not found"})
        tree = ET.parse(response)
        root = tree.getroot()
        fields = []
        """
        foundElems = root.findall
        ("./data:NXentry/[data:user_name='"+search+"']", nameSpace)
        """
        path = "//*[contains(data:user_name,'"+search+"')]"
        foundElems = root.xpath(path, namespaces=nameSpace)
        for element in foundElems:
            runData = {}
            for data in element:
                dataId = data.tag.replace(
                    '{http://definition.nexusformat.org/schema/3.0}', '')
                try:
                    dataValue = data.text.strip()
                except Exception:
                    dataValue = data.text
                runData[dataId] = dataValue
            fields.append(runData)
        allFields += (fields)
        print(len(foundElems))

    endTime = datetime.now()
    print(endTime - startTime)
    return jsonify(allFields)

# Search all cycles with specified field


@app.route('/getAllJournals/<instrument>/<field>/<search>')
def getAllFieldJournals(instrument, field, search):
    allFields = []
    nameSpace = {'data': 'http://definition.nexusformat.org/schema/3.0'}
    cycles = literal_eval(getCycles(instrument).get_data().decode())
    cycles.pop(0)

    startTime = datetime.now()

    for cycle in (cycles):
        print(instrument, " ", cycle)
        url = 'http://data.isis.rl.ac.uk/journals/ndx' + \
            instrument+'/'+str(cycle)
        try:
            response = urlopen(url)
        except Exception:
            return jsonify({"response": "ERR. url not found"})
        tree = ET.parse(response)
        root = tree.getroot()
        fields = []
        path = "//*[contains(data:"+field+",'"+search+"')]"
        foundElems = root.xpath(path, namespaces=nameSpace)
        for element in foundElems:
            runData = {}
            for data in element:
                dataId = data.tag.replace(
                    '{http://definition.nexusformat.org/schema/3.0}', '')
                try:
                    dataValue = data.text.strip()
                except Exception:
                    dataValue = data.text
                runData[dataId] = dataValue
            fields.append(runData)
        allFields += (fields)
        print(len(foundElems))

    endTime = datetime.now()
    print(endTime - startTime)
    return jsonify(allFields)

# Go to functionality


@app.route('/getGoToCycle/<instrument>/<search>')
def getGoToCycle(instrument, search):
    nameSpace = {'data': 'http://definition.nexusformat.org/schema/3.0'}
    cycles = literal_eval(getCycles(instrument).get_data().decode())
    cycles.pop(0)

    startTime = datetime.now()
    for cycle in (cycles):
        print(instrument, " ", cycle)
        url = 'http://data.isis.rl.ac.uk/journals/ndx' + \
            instrument+'/'+str(cycle)
        try:
            response = urlopen(url)
        except Exception:
            return jsonify({"response": "ERR. url not found"})
        tree = ET.parse(response)
        root = tree.getroot()
        path = "//*[data:run_number="+search+"]"
        foundElems = root.xpath(path, namespaces=nameSpace)
        if(len(foundElems) > 0):
            endTime = datetime.now()
            print(endTime - startTime)
            return cycle
        print(len(foundElems))

    endTime = datetime.now()
    print(endTime - startTime)
    return "Not Found"

# Close server


@app.route('/shutdown', methods=['GET'])
def shutdown():
    shutdown_server()
    return jsonify({"response": "Server shut down"})


if __name__ == '__main__':
    app.run()
