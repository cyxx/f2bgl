
SDL_CFLAGS = `sdl2-config --cflags`
SDL_LIBS = `sdl2-config --libs`

LIBS = $(SDL_LIBS) -lGL -lz -lWildMidi -lfluidsynth -lm

#LTO = -flto
CXXFLAGS += -Wall -Wno-sign-compare -Wpedantic -MMD $(FFMPEG_CFLAGS) $(SDL_CFLAGS) $(LTO)
LDFLAGS  += $(LTO)

SRCS = cabinet.cpp camera.cpp collision.cpp cutscene.cpp cutscenepsx.cpp decoder.cpp file.cpp \
	font.cpp game.cpp icons.cpp input.cpp installer.cpp inventory.cpp main.cpp mdec.cpp menu.cpp \
	mixer.cpp opcodes.cpp raycast.cpp render.cpp resource.cpp saveload.cpp scaler.cpp \
	screenshot.cpp sound.cpp spritecache.cpp stub.cpp texturecache.cpp \
	trigo.cpp util.cpp xmiplayer.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

f2bgl: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o *.d

-include $(DEPS)
