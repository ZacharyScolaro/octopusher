octopusher: *.c
	python3 parse_assets.py
	python3 parse_title_screen.py
	arm-none-eabi-gcc -r -mlong-calls -fno-exceptions -march=armv6-m *.c -ooctopusher.bin -O2 -Wall -lgcc
	arm-none-eabi-strip octopusher.bin -d -R.comment -R.ARM.attributes
	../../Gopher2600/gopher2600 octopusher.bin
