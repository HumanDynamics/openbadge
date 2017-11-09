import csv
#from itertools import izip

'''
next_value = None
for line_cent, line_bom in izip(open('badge_03v4_centroid.csv'), open('badge_03v4_bom.csv')):
	print line_cent.rstrip()

	#print line_bom
'''
bom = {}
with open('badge_03v5_centroid.csv', 'rb') as csvfile:
	with open('badge_03v5_bom.csv', 'rb') as csvfile2:
		
		bomreader = csv.reader(csvfile2, delimiter=',', quotechar='|')
		for (row) in (bomreader):
			#print row[0]
			bom[row[0]]=row[2]
		#print bom

		centreader = csv.reader(csvfile, delimiter=',', quotechar='|')
		for row in centreader:
			
			if len(row) >1:
				part=row[0] 
				if part in bom:
					print ','.join(row)+","+bom[part]
				else:
					print ','.join(row)+",DigiKey"
			else:
				print ','.join(row)
