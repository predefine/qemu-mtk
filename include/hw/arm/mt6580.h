#pragma once
#include "qemu/osdep.h"
#include "hw/boards.h"
#include "cpu-qom.h"

#define TYPE_MT6580 "mt6580"

#define TYPE_MT6580_MACHINE MACHINE_TYPE_NAME(TYPE_MT6580)

#define MT6580_MACHINE(obj) OBJECT_CHECK(MT6580MachineState, (obj), TYPE_MT6580_MACHINE)

typedef struct {
    MachineClass parent;
} MT6580MachineClass;

typedef struct {
	MachineState parent;
	ARMCPU *cpu;
} MT6580MachineState;
