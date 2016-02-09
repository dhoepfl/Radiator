CXXFLAGS=-funsigned-char -std=c++0x

OBJECTS=main.o device.o debug.o serial.o surveillance.o cp850_to_utf8.o output.o

SRCDIR=src
BINNAME=radiator
BUILDDIR=_build
RELEASESOURCE=release.in
RELEASEDIR=release

DESTROY=$(shell tput setaf 1)
PREPARE=$(shell tput setaf 2)
WORK=$(shell tput setaf 4)
NORMAL=$(shell tput setaf 0)

.PHONY: clean release

all: ${BINNAME}

clean:
	@echo "[${DESTROY}CLEAN${NORMAL}]"
	@rm -f $(OBJECTS:%=${BUILDDIR}/%)
	@rm -f "${BUILDDIR}/${BINNAME}"
	@rm -f "${BINNAME}"
	@rm -fr "${RELEASEDIR}"
	@[ \! -e "${BUILDDIR}" ] || rmdir "${BUILDDIR}"

${BUILDDIR}:
	@echo "[${PREPARE}PREPARE${NORMAL}]"
	@[ -e "${BUILDDIR}" ] || mkdir -p ${BUILDDIR}

${BUILDDIR}/%.o: ${SRCDIR}/%.cpp ${SRCDIR}/%.h | $(filter-out $(wildcard ${BUILDDIR}), ${BUILDDIR}) ${BUILDDIR}
	@echo "[${WORK}CC${NORMAL}] $@"
	@${CXX} -g -c -o "$@" ${CXXFLAGS} "${SRCDIR}/$*.cpp"

${BUILDDIR}/%.o: ${SRCDIR}/%.cpp | $(filter-out $(wildcard ${BUILDDIR}), ${BUILDDIR}) ${BUILDDIR}
	@echo "[${WORK}CC${NORMAL}] $@"
	@${CXX} -g -c -o "$@" ${CXXFLAGS} "${SRCDIR}/$*.cpp"

${BUILDDIR}/${BINNAME}: $(OBJECTS:%.o=${BUILDDIR}/%.o)
	@echo "[${WORK}LD${NORMAL}] $@"
	@${CXX} -g -o "$@" $^

${BINNAME}: ${BUILDDIR}/${BINNAME}
	@echo "[${PREPARE}RELEASE${NORMAL}] $@"
	@ln -f -s "${BUILDDIR}/${BINNAME}" "$@"

release: ${BINNAME}
	@echo "[${PREPARE}RELEASE${NORMAL}] ${RELEASEDIR}"
	@cp -r -a "${RELEASESOURCE}" "${RELEASEDIR}"
	@mkdir -p "${RELEASEDIR}/bin"
	@mkdir -p "${RELEASEDIR}/etc/init.d"
	@mkdir -p "${RELEASEDIR}/scripts"
	@mkdir -p "${RELEASEDIR}/var/run"
	@mkdir -p "${RELEASEDIR}/var/log"
	@cp "$^" "${RELEASEDIR}/bin/"


