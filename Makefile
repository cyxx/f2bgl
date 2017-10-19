
SDL_CFLAGS = `sdl2-config --cflags`
SDL_LIBS = `sdl2-config --libs`

#DEFINES = -DF2B_DEBUG

LIBS = $(SDL_LIBS) -lGL -lz -lWildMidi -lfluidsynth -lm

CXXFLAGS += -Wall -Wno-sign-compare

SRCS = cabinet.cpp camera.cpp collision.cpp cutscene.cpp decoder.cpp file.cpp \
	font.cpp game.cpp icons.cpp input.cpp installer.cpp inventory.cpp main.cpp menu.cpp mixer.cpp \
	opcodes.cpp raycast.cpp render.cpp resource.cpp saveload.cpp scaler.cpp \
	screenshot.cpp sound.cpp spritecache.cpp stub.cpp texturecache.cpp \
	trigo.cpp util.cpp xmiplayer.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

CXXFLAGS += -MMD $(DEFINES) $(SDL_CFLAGS)

f2bgl: $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

clean:
	rm -f *.o *.d

-include $(DEPS)
