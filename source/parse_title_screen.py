from math import sqrt
import png

y = 0;

def parse_2600():
	global y
	y = 0
	pf_color = (0 << 16) | (44 << 8) | 92
	sprite_color = 0xffffff

	pf0=[]
	pf1=[]
	pf2=[]

	sprites = []


	for i in range(0,6):
		 sprites.append([])

	def is_pixel_pf(row, x):
		pixel = (row[x*24] << 16) | (row[x*24 + 1] << 8) | row[x*24+2]
		return pixel == pf_color

	def is_pixel_sprite(row, x):
		global y
		if y < 64:
			offset = 180
			size = 12
		else:
			offset = 336
			size = 6
		pixel = (row[x*size + offset] << 16) | (row[x*size + offset + 1] << 8) | row[x*size+ offset + 2]
		return pixel == sprite_color

	f = open('../assets/title_screen_2600.png', 'rb')
	r = png.Reader(f)
	width, height, rows, info = r.read()

	for row in rows:
		pf0.append( (0x10 if is_pixel_pf(row, 0) else 0) | (0x20 if is_pixel_pf(row, 1) else 0) \
					| (0x40 if is_pixel_pf(row, 2) else 0) | (0x80 if is_pixel_pf(row, 3) else 0))

		pf1.append( (0x01 if is_pixel_pf(row, 11) else 0) | (0x02 if is_pixel_pf(row, 10) else 0) \
					| (0x04 if is_pixel_pf(row, 9) else 0) | (0x08 if is_pixel_pf(row, 8) else 0) \
					| (0x10 if is_pixel_pf(row, 7) else 0) | (0x20 if is_pixel_pf(row, 6) else 0) \
					| (0x40 if is_pixel_pf(row, 5) else 0) | (0x80 if is_pixel_pf(row, 4) else 0) )

		pf2.append( (0x01 if is_pixel_pf(row, 12) else 0) | (0x02 if is_pixel_pf(row, 13) else 0) \
					| (0x04 if is_pixel_pf(row, 14) else 0) | (0x08 if is_pixel_pf(row, 15) else 0) \
					| (0x10 if is_pixel_pf(row, 16) else 0) | (0x20 if is_pixel_pf(row, 17) else 0) \
					| (0x40 if is_pixel_pf(row, 18) else 0) | (0x80 if is_pixel_pf(row, 19) else 0) )

		for i in range(0,6):
			 sprites[i].append((0x01 if is_pixel_sprite(row, 7+i*8) else 0) | (0x02 if is_pixel_sprite(row, 6+i*8) else 0) \
					| (0x04 if is_pixel_sprite(row, 5+i*8) else 0) | (0x08 if is_pixel_sprite(row, 4+i*8) else 0) \
					| (0x10 if is_pixel_sprite(row, 3+i*8) else 0) | (0x20 if is_pixel_sprite(row, 2+i*8) else 0) \
					| (0x40 if is_pixel_sprite(row, 1+i*8) else 0) | (0x80 if is_pixel_sprite(row, 0+i*8) else 0))

		y+=1


	f.close()

	f_header = open('title_screen_2600.h', 'wt')
	f_header.write('#include <stdint.h>\r\n')
	f_header.write('extern const int8_t TitleScreen2600Graphics[192*9];\r\n')
	f_header.close()

	f_out = open('title_screen_2600.c', 'wt')
	f_out.write('#include "title_screen_2600.h"\r\n')
	f_out.write('const int8_t TitleScreen2600Graphics[192*9] = {\r\n')

	for i in range(0,192):
		f_out.write(hex(pf0[i]))
		f_out.write(', ')
		f_out.write(hex(pf1[i]))
		f_out.write(', ')
		f_out.write(hex(pf2[i]))
		f_out.write(',')
		for j in range(0,6):
			f_out.write(hex(sprites[j][i]))
			if j < 5:
				f_out.write(',')
		if i < 191:
			f_out.write(',')
		f_out.write('\r\n')

	f_out.write('};\r\n')

	f_out.close()

def parse_7800():
	pixel_bytes = []
	palette = [ ]
	left_pixel = [ 0x00, 0x40, 0x80, 0xc0, 0x44, 0x84, 0xc4, 0x48, 0x88, 0xc8, 0x4c, 0x8c, 0xcc ]
	right_pixel = [ 0x00, 0x10, 0x20, 0x30, 0x11, 0x21, 0x31, 0x12, 0x22, 0x32, 0x13, 0x23, 0x33 ]
	f = open('../assets/title_screen_7800.png', 'rb')
	r = png.Reader(f)
	width, height, rows, info = r.read()

	for row in rows:
		for i in range(0,160, 2):
			rgb = (row[i*8], row[i*8+1], row[i*8+2])
			if rgb not in palette:
				palette.append(rgb)
			cl = palette.index(rgb)
			i+=1
			rgb = (row[i*8], row[i*8+1], row[i*8+2])
			if rgb not in palette:
				palette.append(rgb)
			cr = palette.index(rgb)
			pixel_bytes.append(left_pixel[cl] | right_pixel[cr]);
	f.close()

	palette_output = [ 0xa0, 0xa4, 0x0f, 0xa9, 0x00, 0xfd, 0x2a, 0x28, 0x00, 0xa2, 0xa5, 0x4d, 0x00, 0x4a, 0x54, 0x48 ]

	f_header = open('title_screen_7800.h', 'wt')
	f_header.write('#include <stdint.h>\r\n')
	f_header.write('extern const uint8_t TitleScreen7800Palette[16];\r\n')
	f_header.write('extern const uint8_t TitleScreen7800Graphics[192*80];\r\n')
	f_header.close()

	f_out = open('title_screen_7800.c', 'wt')
	f_out.write('#include "title_screen_7800.h"\r\n')
	f_out.write('const uint8_t TitleScreen7800Palette[16] = { ')
	for i in range(0,16):
		f_out.write("{0:#0{1}x}".format(palette_output[i], 4))
		if i < 15:
			f_out.write(', ')
	f_out.write(' };\r\n')


	f_out.write('const uint8_t TitleScreen7800Graphics[192*80] = {')
	for i in range(0,len(pixel_bytes)):
		if i % 80 == 0:
			f_out.write('\r\n')
		f_out.write("{0:#0{1}x}".format(pixel_bytes[i], 4))
		if i < len(pixel_bytes)-1:
			f_out.write(',')
	f_out.write('\r\n};\r\n')

	f_out.close()


parse_2600()
parse_7800()