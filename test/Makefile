all: hookmain libhooklib2.so

hookmain: hookmain.c libhooktest.so libhooklib1.so
	gcc -g -o hookmain hookmain.c -lhooktest -lhooklib1 -ldl -L.

libhooktest.so: hooktest.c
	gcc -g -o $@ $< -shared

libhooklib1.so: hooklib1.c
	gcc -g -o $@ $< -shared

libhooklib2.so: hooklib2.c libhooklib3.so
	gcc -g -o $@ $< -lhooklib3 -shared -L.

libhooklib3.so: hooklib3.c
	gcc -g -o $@ $< -shared

clean:
	rm -f hookmain libhooktest.so libhooklib1.so libhooklib2.so libhooklib3.so
