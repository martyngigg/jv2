# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 E. Devlin and T. Youngs

from flask import Flask
from flask import jsonify
from flask import request

from urllib.request import urlopen
from xml.etree.ElementTree import parse

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

# Close server


@app.route('/shutdown', methods=['GET'])
def shutdown():
    shutdown_server()
    return jsonify({"response": "Server shut down"})


if __name__ == '__main__':
    app.run()
