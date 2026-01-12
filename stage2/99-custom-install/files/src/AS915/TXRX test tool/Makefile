all: freqset6 freqset8 freqset6_915 freqset8_915

freqset8_915: freqset8.c rffc2071a_gateway_915_8ch.o
	gcc freqset8.c -g -o $@ rffc2071a_gateway_915_8ch.o -lgpiod -lm

freqset6_915: freqset6.c rffc2071a_gateway_915_6ch.o
	gcc freqset6.c -g -o $@ rffc2071a_gateway_915_6ch.o -lgpiod -lm

freqset6: freqset6.c rffc2071a_gateway_6.o
	gcc $@.c -g -o $@ rffc2071a_gateway_6.o -lgpiod -lm

freqset8: freqset8.c rffc2071a_gateway_8.o
	gcc $@.c -g -o $@ rffc2071a_gateway_8.o -lgpiod -lm

.c.o:
	gcc -g -c $^

clean:
	rm -f *.o freqset6 freqset8 freqset_915
