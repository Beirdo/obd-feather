#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/j1850_process.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/j1850_process.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=i2c_slave.c interrupt.c main.c receiver.c ringbuf.c transmitter.c config_bits.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/i2c_slave.p1 ${OBJECTDIR}/interrupt.p1 ${OBJECTDIR}/main.p1 ${OBJECTDIR}/receiver.p1 ${OBJECTDIR}/ringbuf.p1 ${OBJECTDIR}/transmitter.p1 ${OBJECTDIR}/config_bits.p1
POSSIBLE_DEPFILES=${OBJECTDIR}/i2c_slave.p1.d ${OBJECTDIR}/interrupt.p1.d ${OBJECTDIR}/main.p1.d ${OBJECTDIR}/receiver.p1.d ${OBJECTDIR}/ringbuf.p1.d ${OBJECTDIR}/transmitter.p1.d ${OBJECTDIR}/config_bits.p1.d

# Object Files
OBJECTFILES=${OBJECTDIR}/i2c_slave.p1 ${OBJECTDIR}/interrupt.p1 ${OBJECTDIR}/main.p1 ${OBJECTDIR}/receiver.p1 ${OBJECTDIR}/ringbuf.p1 ${OBJECTDIR}/transmitter.p1 ${OBJECTDIR}/config_bits.p1

# Source Files
SOURCEFILES=i2c_slave.c interrupt.c main.c receiver.c ringbuf.c transmitter.c config_bits.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/j1850_process.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=16F15225
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/i2c_slave.p1: i2c_slave.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/i2c_slave.p1.d 
	@${RM} ${OBJECTDIR}/i2c_slave.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/i2c_slave.p1 i2c_slave.c 
	@-${MV} ${OBJECTDIR}/i2c_slave.d ${OBJECTDIR}/i2c_slave.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/i2c_slave.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/interrupt.p1: interrupt.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/interrupt.p1.d 
	@${RM} ${OBJECTDIR}/interrupt.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/interrupt.p1 interrupt.c 
	@-${MV} ${OBJECTDIR}/interrupt.d ${OBJECTDIR}/interrupt.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/interrupt.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.p1.d 
	@${RM} ${OBJECTDIR}/main.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/main.p1 main.c 
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/receiver.p1: receiver.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/receiver.p1.d 
	@${RM} ${OBJECTDIR}/receiver.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/receiver.p1 receiver.c 
	@-${MV} ${OBJECTDIR}/receiver.d ${OBJECTDIR}/receiver.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/receiver.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/ringbuf.p1: ringbuf.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ringbuf.p1.d 
	@${RM} ${OBJECTDIR}/ringbuf.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/ringbuf.p1 ringbuf.c 
	@-${MV} ${OBJECTDIR}/ringbuf.d ${OBJECTDIR}/ringbuf.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/ringbuf.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/transmitter.p1: transmitter.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/transmitter.p1.d 
	@${RM} ${OBJECTDIR}/transmitter.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/transmitter.p1 transmitter.c 
	@-${MV} ${OBJECTDIR}/transmitter.d ${OBJECTDIR}/transmitter.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/transmitter.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/config_bits.p1: config_bits.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/config_bits.p1.d 
	@${RM} ${OBJECTDIR}/config_bits.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/config_bits.p1 config_bits.c 
	@-${MV} ${OBJECTDIR}/config_bits.d ${OBJECTDIR}/config_bits.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/config_bits.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
else
${OBJECTDIR}/i2c_slave.p1: i2c_slave.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/i2c_slave.p1.d 
	@${RM} ${OBJECTDIR}/i2c_slave.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/i2c_slave.p1 i2c_slave.c 
	@-${MV} ${OBJECTDIR}/i2c_slave.d ${OBJECTDIR}/i2c_slave.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/i2c_slave.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/interrupt.p1: interrupt.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/interrupt.p1.d 
	@${RM} ${OBJECTDIR}/interrupt.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/interrupt.p1 interrupt.c 
	@-${MV} ${OBJECTDIR}/interrupt.d ${OBJECTDIR}/interrupt.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/interrupt.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.p1.d 
	@${RM} ${OBJECTDIR}/main.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/main.p1 main.c 
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/receiver.p1: receiver.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/receiver.p1.d 
	@${RM} ${OBJECTDIR}/receiver.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/receiver.p1 receiver.c 
	@-${MV} ${OBJECTDIR}/receiver.d ${OBJECTDIR}/receiver.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/receiver.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/ringbuf.p1: ringbuf.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ringbuf.p1.d 
	@${RM} ${OBJECTDIR}/ringbuf.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/ringbuf.p1 ringbuf.c 
	@-${MV} ${OBJECTDIR}/ringbuf.d ${OBJECTDIR}/ringbuf.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/ringbuf.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/transmitter.p1: transmitter.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/transmitter.p1.d 
	@${RM} ${OBJECTDIR}/transmitter.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/transmitter.p1 transmitter.c 
	@-${MV} ${OBJECTDIR}/transmitter.d ${OBJECTDIR}/transmitter.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/transmitter.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/config_bits.p1: config_bits.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/config_bits.p1.d 
	@${RM} ${OBJECTDIR}/config_bits.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/config_bits.p1 config_bits.c 
	@-${MV} ${OBJECTDIR}/config_bits.d ${OBJECTDIR}/config_bits.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/config_bits.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/j1850_process.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/j1850_process.${IMAGE_TYPE}.map  -D__DEBUG=1  -mdebugger=none  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto        $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/j1850_process.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}     
	@${RM} ${DISTDIR}/j1850_process.${IMAGE_TYPE}.hex 
	
else
${DISTDIR}/j1850_process.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/j1850_process.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -O0 -fasmfile -maddrqual=ignore -xassembler-with-cpp -v -mwarn=-3 -Wa,-a -msummary=+psect,+class,+mem,+hex,-file -mcodeoffset=0x0400  -ginhx32 -Wl,--data-init -mkeep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/j1850_process.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}     
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${OBJECTDIR}
	${RM} -r ${DISTDIR}

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell "${PATH_TO_IDE_BIN}"mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
