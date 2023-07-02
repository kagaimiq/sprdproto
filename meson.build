project('sprdproto', 'c')

dep_libusb = dependency('libusb-1.0')

srcs = [
    'sprdproto.c',
    'sprd_io.c',
    'sprd_usbdev.c'
]

executable('sprdrunner',
    sources: srcs,
    dependencies: dep_libusb
)
