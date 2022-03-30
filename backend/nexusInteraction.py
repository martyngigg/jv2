# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin and T. Youngs

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


def runFields(instrument, cycles, runs):
    runArr = runs.split(";")
    cycleArr = cycles.split(";")
    nxsFile = file(instrument, cycleArr[0], runArr[0])
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


def getSpectrum(instrument, cycle, runs, spectra):
    data = [[runs, spectra, "detector"]]
    for run in runs.split(";"):
        nxsFile = file(instrument, cycle, run)
        mainGroup = nxsFile['raw_data_1']
        runData = []
        time_of_flight = mainGroup["detector_1"]["time_of_flight"]
        counts = mainGroup["detector_1"]["counts"][0][int(spectra)]
        runData += list(zip(time_of_flight.astype('float64'),
                        counts.astype('float64')))
        data.append(runData)
    return data


def getMonSpectrum(instrument, cycle, runs, monitor):
    data = [[runs, monitor, "monitor"]]
    for run in runs.split(";"):
        nxsFile = file(instrument, cycle, run)
        mainGroup = nxsFile['raw_data_1']
        runData = []
        time_of_flight = mainGroup["monitor_"+monitor]["time_of_flight"]
        counts = mainGroup["monitor_"+monitor]["data"][0][0]
        runData += list(zip(time_of_flight.astype('float64'),
                        counts.astype('float64')))
        data.append(runData)
    return data


def getSpectrumRange(instrument, cycle, runs):
    run = runs.split(";")[0]
    nxsFile = file(instrument, cycle, run)
    mainGroup = nxsFile['raw_data_1']
    spectraCount = len(mainGroup["detector_1"]["counts"][0])
    return spectraCount


def getMonitorRange(instrument, cycle, runs):
    monRange = "0"
    run = runs.split(";")[0]
    nxsFile = file(instrument, cycle, run)
    mainGroup = nxsFile['raw_data_1']
    for key in mainGroup.keys():
        if "monitor" in key:
            if key.split("_")[1] > monRange and key.split("_")[1].isdigit():
                monRange = key.split("_")[1]
    return int(monRange)


def detectorAnalysis(instrument, cycle, run):
    nxsFile = file(instrument, cycle, run)
    detectors = nxsFile['raw_data_1']["detector_1"]["counts"][0]
    count = 0
    for detector in detectors:
        if any(detector):
            count += 1
    return str(count) + "/" + str(len(detectors))


if __name__ == '__main__':
    print("activated")
    nxsFile = file("nimrod", "cycle_20_3", "71158")
    mainGroup = nxsFile['raw_data_1']
    for value in mainGroup['monitor_8']['data'][0][0]:
        if (value > 0):
            print(value)
