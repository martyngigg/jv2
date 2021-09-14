from flask import Flask
from flask import jsonify

from urllib.request import urlopen
from xml.etree.ElementTree import parse

app = Flask(__name__)

@app.route('/getCycles/<instrument>')
def getCycles(instrument):
    url = 'http://data.isis.rl.ac.uk/journals/ndx'+instrument+'/journal_main.xml'
    try:
        response = urlopen(url)
    except:
        return jsonify({"response":"ERR. url not found"})
    tree = parse(response)
    root = tree.getroot()
    cycles = []
    for data in root:
        cycles.append(data.get('name'))
    return jsonify(cycles)

@app.route('/getJournal/<instrument>/<cycle>')
def getJournal(instrument, cycle):
    url = 'http://data.isis.rl.ac.uk/journals/ndx'+instrument+'/'+cycle
    try:
        response = urlopen(url)
    except:
        return jsonify({"response":"ERR. url not found"})
    tree = parse(response)
    root = tree.getroot()
    fields = []
    for run in root:
        runData={}
        for data in run:
            dataId = data.tag.replace('{http://definition.nexusformat.org/schema/3.0}','')
            try:
                dataValue = data.text.strip()
            except:
                dataValue = data.text
            runData[dataId] = dataValue
        fields.append(runData)
    return jsonify(fields)

@app.route('/getFiltered/<instrument>/<cycle>/<filterValue>')
def getFiltered(instrument, cycle, filterValue):
    try:
        url = 'http://data.isis.rl.ac.uk/journals/ndx'+instrument+'/journal_'+cycle+'.xml'
        response = urlopen(url)
    except:
        return jsonify({"response":"ERR. url not found"})
    tree = parse(response)
    root = tree.getroot()
    fields = []
    for run in root:
        runData={}
        found = False
        for data in run:
            dataId = data.tag.replace('{http://definition.nexusformat.org/schema/3.0}','')
            try:
                dataValue = data.text.strip()
            except:
                dataValue = data.text
            if str(filterValue).lower() in str(dataValue).lower():
                found = True
            runData[dataId] = dataValue
        if found:
            fields.append(runData)
    return jsonify(fields)

if __name__ == '__main__':
 app.run()
