#Thanks to Terin#3772 whom I've stolen lots of the code from his system distancer.
#Thanks to Todd1010 who wrote the original for adding arrival distances, that I'm now modifying for departures.

#Value to add by
offset = 150
minimaldeparture = 100.

file_in = "map systems.txt"
file_read = open(file_in, 'r')
output = open('out' + file_in, 'w')
full = file_read.readlines()
outfull = []
departurewritecount=0
for line in range(len(full)):
      if (full[line].startswith('system')):
            arrival=0
            departurewritten = False
            #outfull.append(full[line])
      if (full[line].startswith('\tarrival')) :
            #pos_base = float(full[line][5:])
            arrivalline = line
            arrival = float(full[line][9:])
            outfull.append(full[line])
            departure = arrival * 0.75
            if(minimaldeparture > departure):
                  departure = minimaldeparture
            departurewritten = True
            departurewritecount+=1
            outfull.insert(arrivalline+(departurewritecount),'\tdeparture '+ str(f"{departure:.2f}")+'\n')
            #output.write(full[line])
      elif(full[line].startswith("\tdeparture")):
            pass
      elif(full[line].startswith("\tobject") and departurewritten == False):
            departure = arrival * 0.75
            if(minimaldeparture > departure):
                  departure = minimaldeparture
            #output.write('\tdeparture '+ str(f"{departure:.2f}")+'\n') 
            departurewritten = True
            departurewritecount+=1
            outfull.insert(arrivalline+(departurewritecount),'\tdeparture '+ str(f"{departure:.2f}")+'\n')
            #output.write(full[line])
            outfull.append(full[line])
      else :
            outfull.append(full[line])
            #output.write(full[line])
output.writelines(outfull)
file_read.close