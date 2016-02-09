CXXFLAGS=-funsigned-char -std=c++0x

OBJECTS=main.o device.o debug.o serial.o surveillance.o cp850_to_utf8.o output.o
BUILDDIR=_build

.PHONY: clean

all: radiator

clean:
	rm -f $(OBJECTS:%=${BUILDDIR}/%)
	rm -f "${BUILDDIR}/radiator"
	rm -f "radiator"
	[ \! -e "${BUILDDIR}" ] || rmdir "${BUILDDIR}"

${BUILDDIR}:
	[ -e "${BUILDDIR}" ] || mkdir -p ${BUILDDIR}

${BUILDDIR}/%.o: %.cpp %.h $(filter-out $(wildcard ${BUILDDIR}), ${BUILDDIR})
	${CXX} -g -c -o "$@" ${CXXFLAGS} "$*.cpp"

${BUILDDIR}/%.o: %.cpp $(filter-out $(wildcard ${BUILDDIR}), ${BUILDDIR})
	${CXX} -g -c -o "$@" ${CXXFLAGS} "$*.cpp"

${BUILDDIR}/radiator: $(OBJECTS:%.o=${BUILDDIR}/%.o)
	${CXX} -g -o "$@" $^

radiator: ${BUILDDIR}/radiator
	ln -f -s "${BUILDDIR}/radiator" "$@"

