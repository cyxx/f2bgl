
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS = `sdl-config --libs`

#DEFINES = -DF2B_DEBUG

LIBS = $(SDL_LIBS) -lGL

CXXFLAGS += -g -O -Wall -Wuninitialized -Wno-sign-compare

SRCS = box.cpp camera.cpp collision.cpp cutscene.cpp decoder.cpp file.cpp \
	font.cpp game.cpp input.cpp inventory.cpp main.cpp mathfuncs.cpp menu.cpp mixer.cpp \
	opcodes.cpp raycast.cpp render.cpp resource.cpp scaler.cpp sound.cpp spritecache.cpp \
	stub.cpp texturecache.cpp util.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

CXXFLAGS += -MMD $(DEFINES) $(SDL_CFLAGS)

f2bgl: $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

clean:
	rm -f *.o *.d

-include $(DEPS)
