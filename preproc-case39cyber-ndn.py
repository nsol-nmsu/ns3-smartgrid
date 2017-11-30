import sys, csv

FLOW_TYPES = {
    #'PMU_AGG' : "/direct/agg/pmu/",
    #'PMU_COM' : "/direct/com/pmu/",
    #'AMI_AGG' : "/direct/agg/ami/",
    #'AMI_COM' : "/direct/com/ami/",
    'WAC'   : "/power/wac/",
    'PDC'   : "/power/pdc/"
    #'DATA'    : "/overlay/com/subscription"
}

def gettype(name):
    for k, v in FLOW_TYPES.items():
        if name.startswith(v):
            return k
    raise Exception("Unclassified name: '%s'" % name)

def main(infile):

    latlog = open("lat_%s" % infile, "w")
    latlog.write("srcid dstid timesent timerecv latency flowcls\n")

    fh = open(infile)
    reader = csv.reader(fh, delimiter=',', skipinitialspace=True)
    reader.next() # skip header
    
    deliveredCount = {x: 0 for x in FLOW_TYPES}
    deliveredSize  = {x: 0 for x in FLOW_TYPES}
    
    outstanding = {}
    agg_queue = {}
    agg_tmp = {}
    
    for nodeid, event, name, payloadsize, time in reader:
    
        nodeid = int(nodeid)
        payloadsize = int(payloadsize)
        time = float(time)
        cls = gettype(name)
        
        if event == "sent":
            
            # pub-sub flow needs different handling due to multisource multicast
            if cls == "DATA":
                outstanding[name] = (time, payloadsize, 1)
            else:
                if name in outstanding:
                    #(t1, ps1, c1) = outstanding[name]
                    #if t1 != time:
                    #    raise Exception("Duplicate outstanding with mismatched timestamp")
                    #outstanding[name] = (t1, ps1 + payloadsize, c1 + 1)
                    raise Exception("whoa dupe")
                else:
                    outstanding[name] = (time, payloadsize, 1, nodeid)
            if cls in ["PMU_COM", "AMI_COM"]:
                agg_tmp[name] = agg_queue[(nodeid, cls[:3])]
                agg_queue[(nodeid, cls)] = []
            
        elif event == "recv":
	    if name in outstanding:
            	t1, ps1, c1, sn1 = outstanding[name]
	    else:
		continue
            
            # don't remove outstanding pub-sub entry, it can be delivered multiple times
            if cls != "DATA":
		pass #single interest packet can be delivered multiple times
                #if c1 == 1:
                    #del outstanding[name]
                #else:
                    #raise Exception("ddd")
                    #outstanding[name] = (t1, ps1 - payloadsize, c1 - 1)
            
            latency = time - t1
            deliveredCount[cls] += 1
            deliveredSize[cls]  += ps1

            latlog.write("%d %d %.9f %.9f %.9f %s\n" % (sn1, nodeid, t1, time, latency, cls))
            
            if cls in ["PMU_AGG", "AMI_AGG"]:
                if not (nodeid, cls) in agg_queue:
                    agg_queue[(nodeid, cls[:3])] = []
                agg_queue[(nodeid, cls[:3])].append(latency)
            
            if cls in ["PMU_COM", "AMI_COM"]:
                for agglat in agg_tmp[name]:
                    latlog.write("%s %.6f\n" % (cls[:3]+ "_TOT", latency + agglat))

    #latlog.close()

    lostCount = {x: 0 for x in FLOW_TYPES}
    lostSize  = {x: 0 for x in FLOW_TYPES}

    metlog = open("met_%s" % infile, "w")
    metlog.write("flowcls recvcnt recvsize losscnt losssize\n")

    #for name, (time, payloadsize, count) in outstanding.items():
        #cls = gettype(name)
        #if cls != "DATA":
            #lostCount[cls] += 1
            #lostSize[cls]  += payloadsize

    #Declare variables to store PMU and AMI total bytes transmitted
    totalLossPMU = 0
    totalRecvPMU = 0
    totalLossAMI = 0
    totalRecvAMI = 0
    totalLossCntPMU = 0
    totalRecvCntPMU = 0
    totalLossCntAMI = 0
    totalRecvCntAMI = 0

    for cls in FLOW_TYPES:
	#Calculate total received and lost bytes (PMU and AMI)
	if cls == "PMU_AGG":
		totalLossPMU += lostSize[cls]
	if cls == "PMU_COM":
		totalLossPMU += lostSize[cls]
		totalRecvPMU += deliveredSize[cls]
                totalLossCntPMU += lostCount[cls]
		totalRecvCntPMU += deliveredCount[cls]
	if cls == "AMI_AGG":
                totalLossAMI += lostSize[cls]
        if cls == "AMI_COM":
                totalLossAMI += lostSize[cls]
                totalRecvAMI += deliveredSize[cls]
		totalLossCntAMI += lostCount[cls]
                totalRecvCntAMI += deliveredCount[cls]

        metlog.write("%s %d %d %d %d\n" % (cls, deliveredCount[cls], deliveredSize[cls], lostCount[cls], lostSize[cls]))
    
    #Write total values received at compute layer to processed file (PMU and AMI)
    metlog.write("%s %d %d %d %d\n" % ("PMU_TOT", totalRecvCntPMU, totalRecvPMU, totalLossCntPMU, totalLossPMU))
    metlog.write("%s %d %d %d %d\n" % ("AMI_TOT", totalRecvCntAMI, totalRecvAMI, totalLossCntAMI, totalLossAMI))

    metlog.close()
    fh.close()

    #add loss packets to the latency file, with very high values (infinity)
    metlognew = open("met_%s" % infile, "r")
    for line in metlognew:
	linevalues = line.split(" ")
	if linevalues[0] == "ERROR":
		for i in range(int(linevalues[3])):
			latlog.write("%s %0.6f\n" % ("ERROR", 0.100000))
	if linevalues[0] == "DATA":
                for i in range(int(linevalues[3])):
                        latlog.write("%s %0.6f\n" % ("DATA", 0.100000))
	if linevalues[0] == "PMU_TOT":
                for i in range(int(linevalues[3])):
                        latlog.write("%s %0.6f\n" % ("PMU_TOT", 0.100000))
	if linevalues[0] == "AMI_TOT":
                for i in range(int(linevalues[3])):
                        latlog.write("%s %0.6f\n" % ("AMI_TOT", 0.100000))

    metlognew.close()
    latlog.close()

    return 0

if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))
