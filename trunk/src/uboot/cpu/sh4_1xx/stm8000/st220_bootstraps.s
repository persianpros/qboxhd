/* bootstrap for st220-0 */
	.org	0x400,0
ENTRY(bootstrap_lx0)
	.byte	 0x01, 0x00, 0x00, 0xb0, 0xbf, 0x41, 0x08, 0x08;
	.byte	 0x00, 0x00, 0x00, 0x95, 0x46, 0x00, 0x00, 0x20;
	.byte	 0xd8, 0x0b, 0x9d, 0x15, 0x80, 0x70, 0x00, 0x88;
	.byte	 0x01, 0x00, 0x00, 0xb0, 0xff, 0xc0, 0x04, 0x08;
	.byte	 0x00, 0x00, 0x00, 0x95, 0x01, 0x00, 0x00, 0xb0;
	.byte	 0x00, 0x00, 0x80, 0x15, 0x3f, 0xc1, 0x03, 0x88;
	.byte	 0x81, 0x00, 0x00, 0xa5, 0x40, 0x01, 0x1b, 0xa0;
	.byte	 0xc0, 0x00, 0x1e, 0x25, 0xc0, 0x20, 0x00, 0x88;
	.byte	 0x00, 0x81, 0x1e, 0xa5, 0xc0, 0x00, 0x1f, 0x25;
	.byte	 0x00, 0x21, 0x00, 0x88, 0x00, 0x00, 0x00, 0x80;
	.byte	 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80;
	.byte	 0x00, 0x00, 0x00, 0x32, 0x05, 0x01, 0x12, 0x25;
	.byte	 0x00, 0x00, 0x00, 0x95, 0x00, 0x00, 0x00, 0xb1;
	.byte	 0xc1, 0x0f, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x80;
	.byte	 0x00, 0x00, 0x00, 0x80, 0xc1, 0x0f, 0x04, 0xa5;
	.byte	 0xc1, 0x0f, 0x0a, 0xa5, 0x00, 0x00, 0x80, 0x31;
	.byte	 0x00, 0x00, 0x80, 0x15, 0x00, 0x01, 0x11, 0xa5;
	.byte	 0x04, 0x01, 0x20, 0x10, 0x04, 0x00, 0x21, 0x10;
	.byte	 0x04, 0x01, 0x21, 0x10;
/* bootstrap for st220-1 */
	.org	0x800,0
ENTRY(bootstrap_lx1)
	.byte	 0x01, 0x00, 0x00, 0xb0, 0xbf, 0x41, 0x08, 0x08;
	.byte	 0x00, 0x00, 0x00, 0x95, 0x46, 0x40, 0x00, 0x20;
	.byte	 0xd8, 0x0b, 0x9d, 0x15, 0x80, 0x70, 0x00, 0x88;
	.byte	 0x01, 0x00, 0x00, 0xb0, 0xff, 0xc0, 0x04, 0x08;
	.byte	 0x00, 0x00, 0x00, 0x95, 0x01, 0x00, 0x00, 0xb0;
	.byte	 0x00, 0x00, 0x80, 0x15, 0x3f, 0xc1, 0x03, 0x88;
	.byte	 0x81, 0x00, 0x00, 0xa5, 0x40, 0x01, 0x1b, 0xa0;
	.byte	 0xc0, 0x00, 0x1e, 0x25, 0xc0, 0x20, 0x00, 0x88;
	.byte	 0x00, 0x81, 0x1e, 0xa5, 0xc0, 0x00, 0x1f, 0x25;
	.byte	 0x00, 0x41, 0x00, 0x88, 0x00, 0x00, 0x00, 0x80;
	.byte	 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80;
	.byte	 0x00, 0x00, 0x00, 0x32, 0x05, 0x01, 0x12, 0x25;
	.byte	 0x00, 0x00, 0x00, 0x95, 0x00, 0x00, 0x00, 0xb1;
	.byte	 0xc1, 0x0f, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x80;
	.byte	 0x00, 0x00, 0x00, 0x80, 0xc1, 0x0f, 0x04, 0xa5;
	.byte	 0xc1, 0x0f, 0x0a, 0xa5, 0x00, 0x00, 0x80, 0x31;
	.byte	 0x00, 0x00, 0x80, 0x15, 0x00, 0x01, 0x11, 0xa5;
	.byte	 0x04, 0x01, 0x20, 0x10, 0x04, 0x00, 0x21, 0x10;
	.byte	 0x04, 0x01, 0x21, 0x10;
/* bootstrap for st220-2 */
	.org	0xc00,0
ENTRY(bootstrap_lx2)
	.byte	 0x01, 0x00, 0x00, 0xb0, 0xbf, 0x41, 0x08, 0x08;
	.byte	 0x00, 0x00, 0x00, 0x95, 0x46, 0x80, 0x00, 0x20;
	.byte	 0xd8, 0x0b, 0x9d, 0x15, 0x80, 0x70, 0x00, 0x88;
	.byte	 0x01, 0x00, 0x00, 0xb0, 0xff, 0xc0, 0x04, 0x08;
	.byte	 0x00, 0x00, 0x00, 0x95, 0x01, 0x00, 0x00, 0xb0;
	.byte	 0x00, 0x00, 0x80, 0x15, 0x3f, 0xc1, 0x03, 0x88;
	.byte	 0x81, 0x00, 0x00, 0xa5, 0x40, 0x01, 0x1b, 0xa0;
	.byte	 0xc0, 0x00, 0x1e, 0x25, 0xc0, 0x20, 0x00, 0x88;
	.byte	 0x00, 0x81, 0x1e, 0xa5, 0xc0, 0x00, 0x1f, 0x25;
	.byte	 0x00, 0x81, 0x00, 0x88, 0x00, 0x00, 0x00, 0x80;
	.byte	 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80;
	.byte	 0x00, 0x00, 0x00, 0x32, 0x05, 0x01, 0x12, 0x25;
	.byte	 0x00, 0x00, 0x00, 0x95, 0x00, 0x00, 0x00, 0xb1;
	.byte	 0xc1, 0x0f, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x80;
	.byte	 0x00, 0x00, 0x00, 0x80, 0xc1, 0x0f, 0x04, 0xa5;
	.byte	 0xc1, 0x0f, 0x0a, 0xa5, 0x00, 0x00, 0x80, 0x31;
	.byte	 0x00, 0x00, 0x80, 0x15, 0x00, 0x01, 0x11, 0xa5;
	.byte	 0x04, 0x01, 0x20, 0x10, 0x04, 0x00, 0x21, 0x10;
	.byte	 0x04, 0x01, 0x21, 0x10;
