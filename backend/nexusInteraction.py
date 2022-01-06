from h5py import File
import os
import platform

# Access nexus file


def file(instrument, cycle, run):
    if platform.system() == "Windows":
        root = "/ISISdata/inst$"

    else:
        root = "isisdata"
    nxsRoot = "/{}/NDX{}/Instrument/data/{}/".format(root,
                                                     instrument.upper(), cycle)
    for root, dir, files in os.walk(nxsRoot):
        for file in files:
            if file.endswith('{}.nxs'.format(run)):
                nxsDir = nxsRoot + (str(file))
                break
    try:
        nxsFile = File(nxsDir)
        return nxsFile
    except(Exception):
        return ["ERR. File failed to open"]

# Get run times


def runTimes(file):
    mainGroup = file['raw_data_1']
    startTime = mainGroup['start_time'][0].decode('UTF-8')
    endTime = mainGroup['end_time'][0].decode('UTF-8')
    return [startTime, endTime]

# Access run data fields


def dataFields(file):
    fields = []
    mainGroup = file['raw_data_1']

    for key in mainGroup.keys():
        if key.endswith('log'):
            blockFields = []
            blockFields.append(key)
            for block in mainGroup[key].values():
                blockFields.append(block.name)
            fields.append(blockFields)
    return fields

# Access run log data


def runData(file, fields, run):
    fieldsArr = fields.split(";")
    runData = []
    runData.append(runTimes(file))
    for field in fieldsArr:
        dataBlock = file[field.replace(":", "/")]
        blockData = []

        if field.__contains__("selog"):
            value = dataBlock['value_log']['value']
            time = dataBlock['value_log']['time']

        elif field.__contains__("runlog"):
            value = dataBlock['value']
            time = dataBlock['time']

        else:
            try:
                value = dataBlock['value_log']['value']
                time = dataBlock['value_log']['time']
            except(Exception):
                try:
                    value = dataBlock['value']
                    time = dataBlock['time']
                except(Exception):
                    return ["response:", "ERR. Data not found"]

        blockShape = value.shape
        blockData.append([run, field])
        for i in range(0, blockShape[0]):
            try:
                blockData.append([float(time[i]), float(value[i])])
            except(Exception):
                blockData.append([float(time[i]), value[i][0].decode('UTF-8')])
        runData.append(blockData)
    return runData

# Get unique fields over all runs


def runFields(instrument, cycle, runs):
    runArr = runs.split(";")
    nxsFile = file(instrument, cycle, runArr[0])
    fields = (dataFields(nxsFile))
    return fields

# Get block data over all runs


def fieldData(instrument, cycle, runs, fields):
    data = []
    runArr = runs.split(";")
    for run in runArr:
        nxsFile = file(instrument, cycle, run)
        data.append(runData(nxsFile, fields, run))
    return data
