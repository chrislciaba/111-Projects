import csv

def main():
    f = open('lab3b_check.txt', 'w')
    bitmap = open('bitmap.csv')
    bitmapReader = list(csv.reader(bitmap))
    group = open('group.csv')
    groupReader = list(csv.reader(group))
    inode = open('inode.csv')
    inodeReader = list(csv.reader(inode))
    dir1 = open('directory.csv')
    dirReader = list(csv.reader(dir1))
    super1 = open('super.csv')
    superReader = list(csv.reader(super1))
    indirect = open("indirect.csv")
    indirectReader = list(csv.reader(indirect))
    #unallocated inode
    inodeBitmapBlock = []
    blockBitmapBlock = []
    for rowB in bitmapReader:
        for rowG in groupReader:
            if rowB[0] == rowG[4]:
                inodeBitmapBlock.append(rowB)
            elif rowB[0] == rowG[5]:
                blockBitmapBlock.append(rowB)
    inodeAllocated = {}
    for rowD in dirReader:
        found = 0
        for rowI in inodeReader:
            if rowD[4] == rowI[0]:
                found = 1
        if found == 0:
            f.write('UNALLOCATED INODE < ' + rowD[4] + ' > REFERENCED BY DIRECTORY < ' + rowD[0] + ' > ENTRY < ' + rowD[1] + ' >\n')
    #missing inodes
    numInodes = int(superReader[0][1])
    numBlocks = int(superReader[0][2])
    ipg = int(superReader[0][6])
    dirInodes = {}
    bitInodes = {}
    dirs = {}
    for inodeD in dirReader:
        dirInodes[inodeD[4]] = 1
    for inodeB in inodeBitmapBlock:
        bitInodes[inodeB[1]] = inodeB[0]
    for i in range(11, numInodes):
        if not (str(i) in dirInodes or str(i) in bitInodes):
            f.write('MISSING INODE < ' + str(i) + ' > SHOULD BE IN FREE LIST < ' + groupReader[i/ipg][4] + ' >\n')
    #this finds incorrect . and .. entries
    for x in dirReader:
        if x[5] == "." and x[0] != x[4]:
            f.write('INCORRECT ENTRY IN < ' + str(x[0]) + ' > NAME < ' + str(x[5]) + ' > LINK TO < ' + str(x[4]) + ' > SHOULD BE < ' + str(x[0]) + ' >\n')
        if x[5] == "..":
            found = 0
            realParent = []
            if int(x[0]) != 2:
                for y in dirReader:
                    if y[5] != "." and y[5] != ".." and y[4] == x[0]:
                        realParent = y
                        if y[0] == x[4]:
                            found = 1
                            break
                if found == 0:
                    f.write('INCORRECT ENTRY IN < ' + str(x[0]) + ' > NAME < ' + str(x[5]) + ' > LINK TO < ' + str(x[4]) + ' > SHOULD BE < ' + str(realParent[0]) + ' >' + '\n')
            else:
                if int(x[4]) != 2:
                    f.write('INCORRECT ENTRY IN < ' + str(x[0]) + ' > NAME < ' + str(x[5]) + ' > LINK TO < ' + str(x[4]) + ' > SHOULD BE < 2 >\n')
    #this part will find the bad link counts
    inodeDict = {}
    for inode in inodeReader:
        inodeDict[inode[0]] = inode
    alreadyUsed = {}
    for x in dirReader:
        count = 0
        if x[5] != "." and x[5] != ".." and not x[4] in alreadyUsed:
            for y in dirReader:
                if x[4] == y[4]:
                    count += 1
            if x[4] in inodeDict:
                a = int(inodeDict[x[4]][5])
                if count != a:
                    alreadyUsed[x[4]] = 1
                    f.write('LINKCOUNT < ' + str(x[4]) + ' > IS < ' + str(a) + ' > SHOULD BE < ' + str(count) + ' >\n')
    #this part will do the free block stuff
    freeBlockBitmap = {}
    bitmapDict = {}
    indirDict = {}
    allocatedBlocks = {}
    multRefBlocks = {}
    multRefKeys = []
    multKeys = []
    for row in groupReader:
        freeBlockBitmap[row[5]] = 1
    for row in bitmapReader:
        if row[0] in freeBlockBitmap:
            bitmapDict[row[1]] = row
    direct = 0
    indirect1 = 0
    indirect2 = 0
    indirect3 = 0
    for row1 in inodeReader:
        found = 0
        for blk in range(11, 25):
            if blk < 23:
                if int(row1[blk], 16) != 0:
                    #multRefBlocks[row1[blk]].append([row1[0], -1, (blk-11)])
                    if not row1[blk] in multRefKeys:
                        multRefBlocks[row1[blk]] = [[row1[0], -1, (blk-11)]]
                        multRefKeys.append(row1[blk])
                    else:
                        multRefBlocks[row1[blk]].append([row1[0], -1, (blk-11)])
                        #multKeys.append(row1[blk])
                if int(row1[blk], 16) >= numBlocks or (int(row1[10]) + 10 >= blk and int(row1[blk], 16) == 0):
                    f.write('INVALID BLOCK < ' + str(row1[blk]) + ' > IN INODE < ' + str(row1[0]) + ' > ENTRY < ' + str(blk - 11)  + ' >'+ '\n')
                if str(int(row1[blk], 16)) in bitmapDict:
                    found = 1
                    f.write('UNALLOCATED BLOCK < ' + str(int(row1[blk], 16)) + ' > REFERENCED BY INODE < ' + row1[0] + ' > ENTRY < ' + str(blk - 11) + ' >'+ '\n')
            elif blk == 23: #single indirect
                for row in indirectReader:
                    if int(row1[blk], 16) != 0 and row1[blk] == row[0]:
                        #multRefBlocks[row[2]].append([row1[0], row[0], int(row[1], 16)])
                        if not row[2] in multRefKeys:
                            multRefBlocks[row[2]] = [[row1[0], row[0], int(row[1], 16)]]
                            multRefKeys.append(row[2])
                        else:
                            multRefBlocks[row[2]].append([row1[0], row[0], int(row[1], 16)])
                            #multKeys.append(row1[blk])
                    if int(row[2], 16) >= numBlocks or (int(row1[10]) + 10 >= blk and int(row1[blk], 16) == 0):
                        f.write('INVALID BLOCK < ' + str(row[0]) + ' > IN INODE < ' + str(row1[0]) + ' > INDIRECT BLOCK < ' + str(row[0]) + ' > ENTRY < ' + str(row[1]) + ' >' + '\n')
                    if int(row[0], 16) == int(row1[blk], 16) and str(int(row[2], 16)) in bitmapDict:
                        f.write('UNALLOCATED BLOCK < ' + str(int(row1[blk], 16)) + ' > REFERENCED BY INODE < ' + row1[0] + ' > INDIRECT BLOCK < ' + str(row1[blk]) + ' > ENTRY < ' + str(blk - 11) + ' >' + '\n')
            elif blk == 24: # double indirect
                doubleIBP = []
                for row in indirectReader:
                    if row1[blk] == row[0]:
                        doubleIBP.append(row[2])
                for bp in doubleIBP:
                    for row in indirectReader:
                        if bp == row[0]:
                            if int(row1[blk], 16) != 0:
                                #multRefBlocks[row[2]].append([row1[0], row[0], int(row[1], 16)])
                                if not row[2] in multRefKeys:
                                    multRefBlocks[row[2]] = [[row1[0], row[0], int(row[1], 16)]]
                                    multRefKeys.append(row[2])
                                else:
                                    #if not [row1[0], row[0], int(row[1], 16)] in multRefBlocks[row[2]]:
                                    multRefBlocks[row[2]].append([row1[0], row[0], int(row[1], 16)])
                            if int(row[2], 16) >= numBlocks: #don't need to check if it's 0 because the previous lab did that
                                f.write('INVALID BLOCK < ' + str(row[0]) + ' > IN INODE < ' + str(row1[0]) + ' > INDIRECT BLOCK < ' + str(row[0]) + ' > ENTRY < ' + str(row[1]) + ' >' + '\n')
                            if str(int(row[2], 16)) in bitmapDict:
                                f.write('UNALLOCATED BLOCK < ' + str(int(row1[blk], 16)) + ' > REFERENCED BY INODE < ' + row1[0] + ' > INDIRECT BLOCK < ' + str(row1[blk]) + ' > ENTRY < ' + str(blk - 11) + ' >' + '\n')
            else: #triple indirect
                doubleIPB = []
                tripleIBP = []
                for row in indirectReader:
                    if row1[blk] == row[0]:
                        doubleIBP.append(row[2])
                for x in doubleIBP:
                    for row in indirectReader:
                        if x == row[0]:
                            tripleIBP.append(row(2))
                for bp in tripleIBP:
                    for row in indirectReader:
                        if bp == row[0]:
                            if int(row1[blk], 16) != 0:
                                #multRefBlocks[row[2]].append([row1[0], row[0], int(row[1], 16)])
                                if not row[2] in multRefKeys:
                                    multRefBlocks[row[2]] = [[row1[0], row[0], int(row[1], 16)]]
                                    multRefKeys.append(row[2])
                                else:
                                    multRefBlocks[row[2]].append([row1[0], row[0], int(row[1], 16)])
                            if int(row[2], 16) >= numBlocks:  #don't need to check if it's 0 because the previous lab did that
                                f.write('INVALID BLOCK < ' + str(row[0]) + ' > IN INODE < ' + str(row1[0]) + ' > INDIRECT BLOCK < ' + str(row[0]) + ' > ENTRY < ' + str(row[1]) + ' >' + '\n')
                            if str(int(row[2], 16)) in bitmapDict:
                                f.write('UNALLOCATED BLOCK < ' + str(int(row1[blk], 16)) + ' > REFERENCED BY INODE < ' + row1[0] + ' > INDIRECT BLOCK < ' + str(row1[blk]) + ' > ENTRY < ' + str(blk - 11) + ' >' + '\n')
    for row in multRefKeys:
        marshmelloIsAwesome = multRefBlocks[row]
        str1 = ''
        if len(marshmelloIsAwesome) > 1:
            str1 = str1 + 'MULTIPLY REFERENCED BLOCK < ' + str(int(row, 16)) + ' > BY'
            for row1 in marshmelloIsAwesome:
                if row1[1] == -1:
                    str1 = str1 + ' INODE < ' + str(row1[0]) + ' > ENTRY < ' + str(row1[2]) + ' >'
                else:
                    tr1 = str1 + ' INODE < ' + str(row1[0]) + ' > INDIRECT BLOCK < ' + str(row1[1]) + ' > ENTRY < ' + str(row1[2]) + ' >'
            f.write(str1 + '\n')

main()
