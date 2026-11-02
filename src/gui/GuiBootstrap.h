// Engine setup for the browser GUI, shared by gui_main.cpp and the GUI tests.
#ifndef TIVIDY_GUIBOOTSTRAP_H
#define TIVIDY_GUIBOOTSTRAP_H

#include "Support.h"  // needs to come before DefinitionRepository.h
#include "ActionRegistry.h"
#include "DefinitionRepository.h"
#include "ProjectRegistry.h"
#include "ReportService.h"
#include "RouterRegistry.h"
#include "WorkflowEngineFacade.h"
#include "WorkflowFactory.h"

struct GuiEngine {
    // order matters: routers must outlive the projects in registry
    RouterRegistry routers;
    ProjectRegistry registry;
    WorkflowFactory factory;
    ReportService reports;
    DefinitionRepository definitions;
    ActionRegistry actions;
    WorkflowEngineFacade facade;

    GuiEngine();

private:
    GuiEngine(const GuiEngine&);
    GuiEngine& operator=(const GuiEngine&);
};

#endif
