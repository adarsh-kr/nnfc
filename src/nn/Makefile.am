AM_CPPFLAGS = $(CXX14_FLAGS) $(THIRD_PARTY_CFLAGS) \
              $(JPEG_CFLAGS) \
              $(EIGEN3_CFLAGS) $(EIGEN3_UNSUPPORTED_CFLAGS)

AM_CXXFLAGS = $(PICKY_CXXFLAGS) $(OPTIMIZATION_FLAGS)

noinst_LIBRARIES = libnn.a

libnn_a_SOURCES = activation.hh activation.cc \
                  convolution.hh convolution.cc \
                  fullyconnected.hh fullyconnected.cc \
                  pool.hh pool.cc \
                  normalization.hh normalization.cc \
                  layers.hh layers.cc \
                  net.hh net.cc
