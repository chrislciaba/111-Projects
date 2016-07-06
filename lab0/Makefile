buildStuff: lab0.c
	gcc -o lab0 lab0.c -g -I.

check:
	echo "Runs correctly" > testin.txt
	./lab0 --input=testin.txt --output=testout.txt
	diff testin.txt testout.txt

clean:
	$(RM) *.txt lab0 *~
dist:
	tar -czvf lab0-004331063.tar.gz Makefile lab0.c README screenshot1.png screenshot2.png
