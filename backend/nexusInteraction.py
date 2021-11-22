from h5py import File
import os

# Access nexus file


def file(instrument, cycle, run):
    nxsRoot = "//ISISdata/inst$/NDX{}/Instrument/data/{}/".format(
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
        return {"response:", "ERR. File failed to open"}

# Access run data fields


def dataFields(file):
    fields = []
    mainGroup = file['raw_data_1']

    for key in mainGroup.keys():
        if key.endswith('log'):
            for block in mainGroup[key].values():
                fields.append(block.name)

    return fields

# Access run log data


def runData(file, fields, run):
    fieldsArr = fields.split(";")
    runData = []
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
                    return {"response:", "ERR. Data not found"}

        blockShape = value.shape
        blockData.append([run, field])
        for i in range(0, blockShape[0]):
            blockData.append([float(time[i]), float(value[i])])
        runData.append(blockData)
    return runData

# Get unique fields over all runs


def runFields(instrument, cycle, runs):
    fields = []
    runArr = runs.split(";")
    for run in runArr:
        nxsFile = file(instrument, cycle, run)
        fields += (dataFields(nxsFile))
    return list(set(fields))

# Get block data over all runs


def fieldData(instrument, cycle, runs, fields):
    data = []
    runArr = runs.split(";")
    for run in runArr:
        nxsFile = file(instrument, cycle, run)
        data.append(runData(nxsFile, fields, run))
    return data
