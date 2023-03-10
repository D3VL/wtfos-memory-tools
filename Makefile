NAME = $(shell cat ./control/control | grep Package | cut -d" " -f2)
ARCH = $(shell cat ./control/control | grep Architecture | cut -d" " -f2)
VERSION = $(shell cat ./control/control | grep Version | cut -d" " -f2)
IPK_NAME = "${NAME}_${VERSION}_${ARCH}.ipk"

all: 
	cd ./jni && ndk-build
	mkdir -p ./data/opt/bin
	cp ./libs/armeabi-v7a/mem-tools ./data/opt/bin

	chmod +x ./control/postinst
	chmod +x ./control/prerm
	chmod +x ./data/opt/bin/mem-tools

	mkdir -p ipk
	echo "2.0" > ipk/debian-binary
	cp -r data ipk/
	cp -r control ipk/
	cd ipk/control && tar --owner=0 --group=0 -czvf ../control.tar.gz .
	cd ipk/data && tar --owner=0 --group=0 -czvf ../data.tar.gz .
	cd ipk/ && tar --owner=0 --group=0 -czvf "./${IPK_NAME}" ./control.tar.gz ./data.tar.gz ./debian-binary

clean:
	rm -rf ./data/opt/bin/mem-tools
	rm -rf ipk
	rm -rf libs
	rm -rf obj
