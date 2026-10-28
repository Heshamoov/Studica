# Build ON THE ROBOT (native g++). Produces ./robot
#   make        build
#   ./run.sh    run (sets the library path for you)
#
# WHY wpilib/ IS BUNDLED: the WPILib libraries already on the board
# (/usr/local/frc/third-party/lib) are roboRIO SOFT-float builds that the
# Pi silently refuses to load. wpilib/ holds the real Raspberry Pi
# (hard-float) builds of the same 2020.3.2 release, EXCEPT libwpiHal.so,
# which is Studica's VMX-pi build (com.kauailabs.vmx.first.hal 2020.3.2-vmxpi-20):
# WPILib's plain Pi HAL is a SIMULATOR and never touches the real pins.
CXX = g++
CXXFLAGS = -std=c++17 -O2 -pthread -Iinclude -Wno-deprecated-declarations -Wno-psabi
SRC = $(wildcard src/*.cpp) $(wildcard src/navx/*.cpp)
OBJ = $(patsubst src/%.cpp,build/%.o,$(SRC))
LIBS = -Llib -lStudica -lStudicaDriver \
       -Lwpilib -lwpilibc -lwpiHal -lntcore -lwpiutil -lcameraserver -lcscore \
       -l:libopencv_core.so.3.4 -l:libopencv_imgproc.so.3.4 -l:libopencv_videoio.so.3.4 -l:libopencv_imgcodecs.so.3.4 \
       -L/usr/local/lib/vmxpi -lvmxpi_hal_cpp -lpthread -ldl -latomic
RPATH = -Wl,-rpath,'$$ORIGIN/wpilib' -Wl,-rpath,/usr/local/lib/vmxpi

robot: $(OBJ)
	$(CXX) -o $@ $(OBJ) -Wl,--start-group $(LIBS) -Wl,--end-group $(RPATH)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build robot
