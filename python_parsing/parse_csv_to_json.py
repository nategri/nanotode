import json

neuronDict = {}

with open('./CElegansNeuronTables/Connectome.csv', 'r') as f:
  for line in f:
    comps = line.split(',')

    if comps[0] == 'Origin':
      continue

    val = int(comps[3])

    if 'GABA' in comps[4]:
      val = -1*val

    try:
      neuronDict[comps[0]][comps[1]] = val
    except KeyError:
      neuronDict[comps[0]] = {comps[1]: val}

    if comps[1] not in neuronDict:
      neuronDict[comps[1]] = {}

with open('./CElegansNeuronTables/NeuronsToMuscle.csv', 'r') as f:
  for line in f:
    comps = line.split(',')

    if comps[0] == 'Neuron':
      continue

    val = int(comps[2])

    if 'GABA' in comps[3]:
      val = -1*val

    try:
      neuronDict[comps[0]][comps[1]] = val
    except KeyError:
      neuronDict[comps[0]] = {comps[1]: val}


    if comps[1] not in neuronDict:
      neuronDict[comps[1]] = {}

outputJson = json.dumps(neuronDict, sort_keys=True, indent=4, separators=(',', ': '))

outputFile = open('connectome.json', 'w')
outputFile.write(outputJson)
outputFile.close()
